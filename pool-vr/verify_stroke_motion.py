"""Numerical regression gate for the original pool stroke, not visual acceptance.

Run with the repository's Python/NumPy/PyFFI environment and locally owned game
skeleton: python -B pool-vr/verify_stroke_motion.py
Default cases cover pose-bank corners, edge midpoints, and reviewed references.
Use --case D,X (repeatable) for a focused reproduction. No animation is exported.
The author module may initialize its ignored, locally owned skeleton cache.
"""
import argparse
import math
import sys

import numpy as np
import stroke_study as stroke

a = stroke.a
STEP = .01
REACH_TOLERANCE = 1e-4
JUMP_DEGREES_PER_10MS = 4.0
WRIST_SWING_LIMIT = 70.0
CASES = [(50, -30), (50, 0), (50, 30),
         (110, -30), (110, 0), (110, 15), (110, 30), (120, 15),
         (180, -30), (180, 0), (180, 30)]
# These bounds catch observed knee locking, elbow whipping, and wrist folding.
# They are regression thresholds, not a clinical or visual anatomy certificate.
BONES = ["Bip01 Spine", "Bip01 Neck", "Bip01 Head"] + [
    "Bip01 " + side + " " + suffix
    for side in ("L", "R")
    for suffix in ("UpperArm", "Forearm", "Hand", "Thigh", "Calf", "Foot",
                   "Thumb1", "Thumb11", "Thumb12")]
LEFT_FINAL_ROTATION = a.rotate([0, 0, 1], math.radians(20)) @ np.array(
    [[0, 0, -1], [1, 0, 0], [0, -1, 0]], dtype=float)
RIGHT_FINAL_ROTATION = np.array(
    [[0, -1, 0], [0, 0, 1], [-1, 0, 0]], dtype=float)


def rotation_angle(matrix):
    return math.degrees(math.acos(float(np.clip((np.trace(matrix)-1)/2, -1, 1))))


def multiply_quaternions(p, q):
    return np.r_[p[3]*q[:3] + q[3]*p[:3] + np.cross(p[:3], q[:3]),
                 p[3]*q[3] - p[:3] @ q[:3]]


def wrist_swing_twist(local, side):
    name = "Bip01 " + side + " Hand"
    # Relative to the hand's neutral local transform, in forearm coordinates.
    q = a.quat(local[name][:3, :3] @ a.bind[name][:3, :3].T)
    axis = a.unit(a.bind[name][:3, 3])
    twist = np.r_[axis*(q[:3] @ axis), q[3]]
    length = np.linalg.norm(twist)
    if length < 1e-9:
        return 180.0, 0.0
    twist /= length
    swing = multiply_quaternions(q, np.r_[-twist[:3], twist[3]])
    degrees = lambda value: math.degrees(2*math.acos(float(np.clip(abs(value), 0, 1))))
    return degrees(swing[3]), degrees(twist[3])


def parse_case(value):
    try:
        d, x = map(int, value.split(","))
    except ValueError as error:
        raise argparse.ArgumentTypeError("Use --case D,X, e.g. --case 110,15") from error
    if not (50 <= d <= 180 and -30 <= x <= 30):
        raise argparse.ArgumentTypeError("Outside authored bank: D50..180 / X-30..30")
    return d, x


def check_case(d, x):
    failures = {}
    peaks = {"clamp": (0.0, 0.0, ""),
             "jump": (0.0, 0.0, ""),
             "wrist_swing": (0.0, 0.0, ""),
             "wrist_twist": (0.0, 0.0, "")}
    current_time = 0.0

    def observe(metric, value, bone, limit=None, detail=""):
        if value > peaks.get(metric, (-1, 0, ""))[0]:
            peaks[metric] = (float(value), current_time, bone)
        if limit is not None and value > limit:
            key = (metric, bone)
            if value > failures.get(key, (-1, ""))[0]:
                failures[key] = (float(value),
                    f"D{d}/X{x} t={current_time:.3f} {bone}: {metric}={value:.5f}"
                    f" limit={limit:.5f} {detail}".rstrip())

    original_ik = a.ik

    def measured_ik(local, upper, middle, tip, target, pole):
        origin = a.world(local, upper)[:3, 3]
        joint = a.world(local, middle)[:3, 3]
        end = a.world(local, tip)[:3, 3]
        reach = np.linalg.norm(np.asarray(target)-origin)
        available = np.linalg.norm(joint-origin)+np.linalg.norm(end-joint)
        original_ik(local, upper, middle, tip, target, pole)
        error = np.linalg.norm(a.world(local, tip)[:3, 3]-target)
        observe("clamp", error, upper, REACH_TOLERANCE,
                f"requested_reach={reach:.5f} limb_length={available:.5f}")

    a.ik = measured_ik
    samples = {}
    try:
        # Adjacent samples are compared only within a contiguous 10-ms sweep.
        windows = [np.arange(141)*STEP, 5.95 + np.arange(126)*STEP]
        for times in windows:
            previous = None
            for time in times:
                current_time = round(float(time), 8)
                local = stroke.pose(current_time, d, x)
                samples[current_time] = local
                for name in BONES:
                    matrix = local[name]
                    if not np.all(np.isfinite(matrix)):
                        observe("nonfinite", 1, name, 0)
                        continue
                    orthogonal_error = np.linalg.norm(matrix[:3, :3].T @ matrix[:3, :3]-np.eye(3))
                    observe("rotation_integrity", orthogonal_error, name, 1e-5)
                    if previous is not None:
                        jump = rotation_angle(matrix[:3, :3] @ previous[name][:3, :3].T)
                        observe("jump", jump, name, JUMP_DEGREES_PER_10MS)
                for side in ("L", "R"):
                    swing, twist = wrist_swing_twist(local, side)
                    observe("wrist_swing", swing, side+" wrist", WRIST_SWING_LIMIT)
                    # Twist is measured separately; it is NOT wrist bending.
                    observe("wrist_twist", twist, side+" wrist")
                previous = local

        for current_time in (1.4, 3.55, 4.3, stroke.CONTACT, 4.88, 5.5):
            local = stroke.pose(current_time, d, x)
            samples[current_time] = local
            for side, expected in (("L", LEFT_FINAL_ROTATION), ("R", RIGHT_FINAL_ROTATION)):
                actual = a.world(local, "Bip01 "+side+" Hand")[:3, :3]
                observe("final_wrist_rotation", rotation_angle(actual @ expected.T),
                        side+" wrist", 1e-4)
            # Frozen calibration anchors: catch accidental edits to the contact
            # pose while repairing transition IK, without pinning each joint.
            bias = np.array([x, d, 53.9])*(1/.95-1)
            goals = {
                "L": np.array([x-2.0, d-28.8, 56.4])+bias,
                "R": np.array([x+3.18328125, d-88-stroke.pull(current_time)-.43,
                              62.43671875+(88+stroke.pull(current_time))*stroke.CUE_SLOPE])+bias}
            for side, expected in goals.items():
                actual = a.world(local, "Bip01 "+side+" Hand")[:3, 3]
                observe("final_wrist_anchor", np.linalg.norm(actual-expected),
                        side+" wrist", 1e-4)

        current_time = stroke.DURATION
        start, end = samples[0.0], samples[7.2]
        for name in start:
            observe("end_pose_return", np.linalg.norm(start[name]-end[name]), name, 1e-5)
        for time in (0.0, 7.2):
            current_time = time
            # Left hand must start/end neutral relative to the forearm.
            relative = samples[time]["Bip01 L Hand"][:3, :3] @ a.bind["Bip01 L Hand"][:3, :3].T
            # NIF bind rotations carry float precision; 0.03 degrees is numerical noise.
            observe("neutral_left_wrist", rotation_angle(relative), "L wrist", .05)
    finally:
        a.ik = original_ik

    summary = " ".join(f"{key}={peaks[key][0]:.3f}@{peaks[key][1]:.2f}"
                       for key in ("clamp", "jump", "wrist_swing", "wrist_twist"))
    print(f"{'FAIL' if failures else 'PASS'} D{d}/X{x}: {summary}", flush=True)
    for _, message in failures.values():
        print("  "+message, flush=True)
    return len(failures)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", action="append", type=parse_case, dest="cases")
    args = parser.parse_args()
    total = sum(check_case(*case) for case in (args.cases or CASES))
    print(f"{'FAIL' if total else 'PASS'} numerical motion gate; {total} failed constraints.")
    print("This does not certify skinned geometry, contact, native playback, or visual quality.")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
