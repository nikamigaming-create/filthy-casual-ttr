"""Assemble a player archive from the verified RC21 payload and release inputs."""

import argparse
import hashlib
import json
from pathlib import Path
import zipfile


RC21_DIGEST = 'b7c60e97280c5fcef3916af012c874eddc6cae56990aea3c14959bae5d359989'
VEHICLE_PLUGIN = 'NVSE/Plugins/FilthyCasualVehicles.dll'
VEHICLE_FUNCTIONS = (
    'AllowCombat', 'FlightSeatLocks', 'ReadFrequency', 'ReadVariable', 'RefreshSeat',
    'SetEnginePitch', 'SetFrequency', 'ShowPilot', 'StartEngineDrive', 'StartEngineIdle',
    'StartSeatIdle', 'StopEngine', 'StopSeatIdle', 'WriteVariable',
)
DEVELOPMENT_DOCS = {
    'docs/COMBINED-TEST.md', 'docs/CURRENT-BETA.md', 'docs/FINAL-TEST.md',
    'docs/VEHICLE-CONTROLS-CURRENT.md', 'vehicle-vr/README.md', 'vehicle-vr/INSTALL.md',
}
ROOT = Path(__file__).resolve().parents[2]
PLAYER_DOCS = (
    'README.md', 'RELEASE-NOTES.md', 'docs/INSTALL.md', 'docs/NATIVE-SETTINGS.md',
    'docs/SPRINT-WRIST-LIGHT.md', 'docs/VEHICLES.md',
)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def player_entry(name):
    return (name not in DEVELOPMENT_DOCS and not name.endswith('.py') and
            not name.startswith(('eyebot/', 'pool-vr/', 'vehicle-vr/',
                                 'NVSE/user_defined_functions/FilthyCasual/Eyebot/')))


def assemble(original, destination, vehicle_dll, version):
    if sha256(original.read_bytes()) != RC21_DIGEST:
        raise ValueError('Expected the exact published RC21 archive.')
    with zipfile.ZipFile(original) as archive:
        manifest = json.loads(archive.read('manifest.json'))
        expected = {row['path']: row['sha256'].lower() for row in manifest['files']}
        names = archive.namelist()
        if len(names) != len(set(names)) or set(names) != set(expected) | {'manifest.json'}:
            raise ValueError('RC21 manifest does not cover the archive.')
        payload = {}
        for name, digest in expected.items():
            content = archive.read(name)
            if sha256(content) != digest:
                raise ValueError(f'RC21 checksum mismatch: {name}')
            if player_entry(name):
                payload[name] = content

    for name in PLAYER_DOCS:
        payload[name] = (ROOT / name).read_bytes()
    for name in VEHICLE_FUNCTIONS:
        relative = f'FilthyCasual/Vehicles/{name}.txt'
        payload[f'NVSE/user_defined_functions/{relative}'] = (ROOT / 'source/native-scripts' / relative).read_bytes()
    if vehicle_dll is not None:
        payload[VEHICLE_PLUGIN] = vehicle_dll.read_bytes()
    payload['meta.ini'] = (
        f'[General]\nmodid=0\nversion={version}\nnewestVersion=\ncategory=0\ninstallationFile=\n'
        'comments=TTR 1.1.7 with xNVSE and JIP LN NVSE. Cars use Uhmatt Drivable Cars 8b. '
        'Vertibird uses Vertibird Player Home 4.18, TTW patch 4.18c, and Pilot Inside. '
        'See README.md for downloads.\n'
    ).encode()
    for key in ('lockedFeatures', 'optionalLocalContent', 'coreOrigin'):
        manifest.pop(key, None)
    manifest['version'] = version
    manifest['payloadOrigin'] = {'version': '1.8.0-beta.4-rc.21', 'sha256': RC21_DIGEST}
    manifest['features'] = [feature for feature in manifest['features'] if feature != 'local-dog-shadow-import']
    manifest['files'] = [{'path': name, 'sha256': sha256(content)} for name, content in sorted(payload.items())]
    payload['manifest.json'] = (json.dumps(manifest, indent=2) + '\n').encode()
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix('.zip.tmp')
    try:
        with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as archive:
            for name, content in sorted(payload.items()):
                entry = zipfile.ZipInfo(name, date_time=(2000, 1, 1, 0, 0, 0))
                archive.writestr(entry, content, compress_type=zipfile.ZIP_DEFLATED)
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rc21', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--vehicle-dll', type=Path)
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    if args.output.suffix.lower() != '.zip' or args.output.resolve() == args.rc21.resolve():
        parser.error('Choose a separate output ZIP.')
    assemble(args.rc21, args.output, args.vehicle_dll, args.version)


if __name__ == '__main__':
    main()
