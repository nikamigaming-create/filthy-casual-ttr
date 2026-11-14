"""Phone-readable Filthy Casuals edit from real-time final-eye recordings.

Default: validate sources and write a reviewable edit plan; never encode video.
Use --prepare-assets for original caption/card PNGs, then --render only after
fresh recordings are reviewed. Render requires --fresh-after; the separately
reviewed gesture pickup is the sole intentionally reusable historical take.
"""
from __future__ import annotations
import argparse
from dataclasses import dataclass, asdict
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'outputs/human-showcase'
WORK = SRC / 'mobile-edit'
OUT = ROOT / 'outputs/Filthy-Casuals-Mobile.mp4'
W, H, FPS = 1080, 1920, 30
EYE = (40, 390, 1000, 1070)
INK, PAPER, CREAM, RED, GOLD = '#183c31', '#efdfb4', '#fff1d1', '#a63828', '#e7a33a'

@dataclass
class Shot:
    key: str
    source: str | None
    start: float
    end: float
    headline: str
    body: str
    control: str
    stamp: str
    card: str = ''
    reusable: bool = False

    @property
    def duration(self):
        return self.end - self.start


def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def events(name):
    path = SRC / f'{name}-timeline.json'
    values = read_json(path)
    if not isinstance(values, list):
        raise ValueError(f'Timeline must be an array: {path}')
    result = {}
    previous = -1.0
    for item in values:
        t = float(item['seconds'])
        if not math.isfinite(t) or t < previous or t < 0:
            raise ValueError(f'Non-monotonic timeline: {path}')
        key = item['action']
        if key in result:
            raise ValueError(f'Ambiguous event {key!r}: {path}')
        result[key] = t
        previous = t
    return result


def required_event(timeline, name):
    if name not in timeline:
        raise ValueError(f'Required capture event is absent: {name}')
    return timeline[name]


def metadata(path, frame_ranges=None):
    sidecar = path.with_suffix('.json')
    d = read_json(sidecar)
    for key in ('pid', 'width', 'height', 'fps', 'encodedFrames', 'repeatedFrames', 'ffmpegExit'):
        if key not in d:
            raise ValueError(f'Missing {key} in {sidecar}')
    if d['ffmpegExit'] != 0 or d['encodedFrames'] <= 0:
        raise ValueError(f'Failed or empty recording: {path}')
    if d['width'] != 2064 or d['height'] != 1104:
        raise ValueError(f'Expected complete 2064x1104 stereo source: {path}')
    if d['fps'] != FPS:
        raise ValueError(f'Expected30fps source: {path}')
    records = d.get('frames', [])
    if records and len(records) != d['encodedFrames']:
        raise ValueError(f'Frame metadata count disagrees: {path}')
    if d['repeatedFrames'] and not records:
        raise ValueError(f'Repeat metadata has no frame records to check freeze duration: {path}')
    longest = run = counted = 0
    for frame in records:
        repeated = bool(frame.get('repeated', False))
        counted += int(repeated)
        run = run + 1 if repeated else 0
        longest = max(longest, run)
    if records and counted != d['repeatedFrames']:
        raise ValueError(f'Repeated-frame summary disagrees with frame records: {path}')
    checked_frames = d['encodedFrames']
    checked_repeats = d['repeatedFrames']
    if frame_ranges is not None:
        # An explicit editorial cut may omit a stationary setup pause. Validate
        # the kept source frames without interpolating or hiding motion stalls.
        if not records or not frame_ranges:
            raise ValueError('Edit ranges require complete per-frame provenance')
        checked_frames = checked_repeats = longest = 0
        previous_end = 0
        for begin, end in frame_ranges:
            if not (isinstance(begin,int) and isinstance(end,int) and previous_end <= begin < end <= len(records)):
                raise ValueError('Invalid or overlapping source edit range')
            run = 0
            for f in records[begin:end]:
                repeated = bool(f.get('repeated', False))
                checked_repeats += int(repeated)
                run = run + 1 if repeated else 0
                longest = max(longest,run)
            checked_frames += end-begin
            previous_end = end
        d['editorialFrameRanges'] = frame_ranges
        d['checkedFrames'] = checked_frames
        d['checkedRepeatedFrames'] = checked_repeats
    fraction = checked_repeats / checked_frames
    d['longestConsecutiveRepeatedFrames'] = longest
    d['repeatedFrameFraction'] = fraction
    if longest > 1 or fraction > .005:
        raise ValueError(f'Capture cadence rejected: {d["repeatedFrames"]}/{d["encodedFrames"]} repeated frames ({fraction:.3%}), longest run {longest}: {path}')
    # Isolated real captured repeats remain intact and visible in metadata.
    # No replacement frames or interpolation are generated to conceal them.
    return d


def duration(path):
    d = metadata(path)
    return d['encodedFrames'] / d['fps']


def footage(key, name, begin, end, headline, body, control, stamp, reusable=False):
    return Shot(key, str(name), begin, end, headline, body, control, stamp, reusable=reusable)


def make_shots(allow_missing=False):
    one = events('01-height-weapons')
    two = events('02-aid-gestures')
    three = events('03-gestures')
    e1 = lambda name: required_event(one, name)
    e2 = lambda name: required_event(two, name)
    e3 = lambda name: required_event(three, name)
    s1, s2, s3 = [SRC / f'{name}-stereo.mp4' for name in ('01-height-weapons', '02-aid-gestures', '03-gestures')]
    shots = [Shot('title', None, 0, 3.5, 'FILTHY\nCASUALS', 'For the couch commanders.', '', 'GET COMFY', card='title')]
    shots.append(footage('height', s1, max(e1('Seated low'), e1('Open height') - 4), e1('Calibrate') + 7.2,
        'YOUR SEAT.\nYOUR HEIGHT.', 'Look down. Get comfortable.', 'LEFT MENU: hold 1 second\nA: calibrate   B: save', 'COZY!'))
    optional = []
    def optional_take(key, filename, limit, headline, body, control, stamp):
        p = SRC / filename
        if not p.exists():
            if not allow_missing:
                raise FileNotFoundError(f'Required pickup missing: {p}; use --allow-missing-optionals only for a partial draft plan')
            optional.append(key)
            return
        try:
            take_duration = duration(p)
        except (ValueError, FileNotFoundError) as error:
            if not allow_missing:
                raise
            optional.append(key + ': invalid/unready source')
            print(f'Draft omits {key}: {error}')
            return
        shots.append(footage(key, p, 0, min(take_duration, limit), headline, body, control, stamp))
    optional_take('height-return', '04-height-return-stereo.mp4', 4.5, 'SET ONCE.\nSTAYS SAVED.',
        'Your seated height returns\nwhen you launch and load.', 'Same chair. Saved height.', 'SAVED!')
    shots.append(footage('weapons', s1, e1('Weapons wheel') + 1.5, e1('Touch weapon') + 6,
        'TOUCH. EQUIP.\nRELAX.', 'Compact previews.\nNormal-size weapon in your right hand.',
        'Hold LEFT thumbrest\nDouble-tap RIGHT thumbrest', 'READY!'))
    shots.append(footage('auto-reload', s1, e1('Automatic reload'), e1('Manual mode'),
        'LESS FUSS.\nMORE PEW.', 'Shoot normally.\nAutomatic mode handles the reload.', 'RIGHT TRIGGER: fire', 'PEW!'))
    shots.append(footage('reload-choice', s1, e1('Manual mode'), e1('Back to auto') + 2,
        'RELOAD\nYOUR WAY.', 'Manual or automatic.\nChange it during play.', 'LEFT MENU: tap to switch', 'YOUR CALL'))
    shots.append(footage('aid', s2, e2('Consumables wheel') + 1.5, e2('Touch consumable') + 6,
        'SNACKS\nWITHIN REACH.', 'Touch to put an item\nin your right hand.',
        'Hold RIGHT thumbrest\nDouble-tap LEFT thumbrest', 'HELD!'))
    optional_take('food-mouth', '06-food-mouth-stereo.mp4', 7, 'SNACK TIME.',
        'Bring sticky-held food to your mouth.', 'Food only. Normal holding stays native.', 'SNACK!')
    optional_take('native-hold', '05-native-hold-stereo.mp4', 7, 'BACK TO\nNATIVE.',
        'Give normal holding control\nback to TTR.', 'RIGHT GRIP: squeeze once', 'YOUR CALL')
    shots.append(footage('clear', s1, e1('Clear weapon') + 4.5, min(e1('Clear weapon') + 10.5, e1('Finished')),
        'HANDS FREE.', 'Touch the big center cap\nto clear your selection.', 'CENTER CAP: clear hand', 'CLEAR!'))
    # Separate unsped gesture holds; each source cut preserves the recognizable pose.
    for key, anchor, length, headline, body, control, stamp in [
        ('curl', 'Gentle finger curl', 3.5, 'A LITTLE\nPERSONALITY.', 'Relax. Curl. Open.', 'Grip and touch drive your hand poses.', 'HELLO!'),
        ('point', 'Point', 4, 'MAKE\nYOUR POINT.', 'Give the gesture a moment.', 'Grip held. Index lifted.', 'LOOK!'),
        ('thumb-up', 'Thumbs up', 4.5, 'COUCH\nAPPROVED.', 'A proper thumbs-up.', 'Lift your thumb. Keep your grip held.', 'NICE!'),
        ('wave', 'Relaxed wave', 3.5, 'TAKE IT\nEASY.', 'Then get back to the game.', 'Relax your grip.', 'CHEERS!')]:
        start = e3(anchor)
        shots.append(footage(key, s3, start, start + length, headline, body, control, stamp, reusable=True))
    for key, filename, limit, headline, body, control, stamp in [
        ('sunny', 'pool-sunny-stroke.mp4', 10, 'YOUR SHOT,\nSUNNY.', 'Pool at the Prospector Saloon.', 'Ask Sunny: Care for a game of pool?', 'GAME ON'),
        ('player-pool', 'pool-player-stroke.mp4', 6.5, 'LINE IT UP.\nLET IT SLIDE.', 'Left hand guides.\nRight hand makes the stroke.', 'Hold BOTH GRIPS\nSlide your right hand forward', 'YOUR TURN')]:
        p = ROOT / 'evidence' / filename
        if not p.exists():
            if not allow_missing:
                raise FileNotFoundError(f'Required new pool take missing: {p}')
            optional.append(key)
            continue
        try:
            take_duration = duration(p)
        except (ValueError, FileNotFoundError) as error:
            if not allow_missing:
                raise
            optional.append(key + ': invalid/unready source')
            print(f'Draft omits {key}: {error}')
            continue
        shots.append(footage(key, p, 0, min(take_duration, limit), headline, body, control, stamp))
    shots += [Shot('install', None, 0, 5.5, 'ADD ZIP.\nENABLE. PLAY.', '', '', 'MO2', card='install'),
              Shot('end', None, 0, 4.5, 'GET COMFY.\nGO PLAY.', "We love A Tale of Two Realities.\nWe're just filthy casuals.", '', 'NIKAMI', card='end')]
    return shots, optional


def sha256(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def validate(shots, fresh_after=None):
    sources = {}
    for s in shots:
        if not math.isfinite(s.start + s.end) or s.start < 0 or s.duration < 2.0:
            raise ValueError(f'Invalid or unreadably short shot: {s.key} {s.start}..{s.end}')
        if not s.source:
            continue
        p = Path(s.source)
        d = metadata(p)
        if s.end > d['encodedFrames'] / d['fps'] + .02:
            raise ValueError(f'Shot extends past recorded frames: {s.key}')
        mtime = datetime.fromtimestamp(p.stat().st_mtime, timezone.utc)
        if fresh_after and not s.reusable and mtime < fresh_after:
            raise ValueError(f'Stale footage rejected for {s.key}: {p} ({mtime.isoformat()})')
        if str(p) not in sources:
            sources[str(p)] = {k: d[k] for k in ('pid', 'fps', 'width', 'height', 'encodedFrames', 'repeatedFrames', 'longestConsecutiveRepeatedFrames', 'repeatedFrameFraction')}
            sources[str(p)].update(sha256=sha256(p), modifiedUtc=mtime.isoformat(),
                historicalReuse=s.reusable, sidecar=str(p.with_suffix('.json')))
    return sources


def graphics():
    from PIL import Image, ImageDraw, ImageFont, ImageOps
    return Image, ImageDraw, ImageFont, ImageOps


def font(size, bold=False):
    _, _, ImageFont, _ = graphics()
    return ImageFont.truetype('C:/Windows/Fonts/' + ('arialbd.ttf' if bold else 'consola.ttf'), size)


def text_block(draw, value, box, size, color=INK, bold=False, gap=8):
    x, y, width, height = box
    f = font(size, bold)
    top = y
    for paragraph in value.split('\n'):
        line = ''
        for word in paragraph.split():
            candidate = (line + ' ' + word).strip()
            if draw.textlength(candidate, font=f) > width and line:
                draw.text((x, y), line, font=f, fill=color)
                y += size + gap
                line = word
            else:
                line = candidate
        if draw.textlength(line, font=f) > width:
            raise ValueError(f'Unbreakable text exceeds box: {line}')
        draw.text((x, y), line, font=f, fill=color)
        y += size + gap
    if y - top - gap > height:
        raise ValueError(f'Text overflow: {value!r} ({y - top - gap}>{height})')
    return y


def background():
    Image, ImageDraw, _, _ = graphics()
    im = Image.new('RGB', (W, H), PAPER)
    draw = ImageDraw.Draw(im)
    for y in range(0, H, 17):
        for x in range((y // 17 % 2) * 8, W, 17):
            draw.ellipse((x, y, x + 2, y + 2), fill='#d7c69e')
    draw.rectangle((18, 18, W - 18, H - 18), outline=INK, width=5)
    return im, draw


def stamp(draw, label, cx=852, cy=1815):
    pts = [(cx + math.cos(i * math.pi / 14) * (166 if i % 2 == 0 else 146),
            cy + math.sin(i * math.pi / 14) * (49 if i % 2 == 0 else 43)) for i in range(28)]
    draw.polygon(pts, fill=GOLD, outline=INK, width=4)
    f = font(25, True)
    draw.text((cx - draw.textlength(label, font=f) / 2, cy - 17), label, font=f, fill=INK)


def caption_asset(s):
    Image, ImageDraw, _, ImageOps = graphics()
    im, draw = background()
    text_block(draw, 'NIKAMI / FILTHY CASUALS', (48, 52, 950, 35), 27, bold=True)
    if not s.card:
        text_block(draw, s.headline, (48, 111, 984, 170), 76, bold=True, gap=2)
        text_block(draw, s.body, (50, 285, 980, 98), 36, gap=7)
        x, y, w, h = EYE
        draw.rectangle((x - 5, y - 5, x + w + 5, y + h + 5), fill=INK)
        draw.rounded_rectangle((40, 1504, 1040, 1735), radius=24, fill=INK)
        text_block(draw, s.control, (70, 1550, 940, 155), 42, color=CREAM, bold=True, gap=15)
        text_block(draw, 'STAY SEATED. STAY COMFORTABLE.', (48, 1785, 630, 70), 23, bold=True)
        stamp(draw, s.stamp)
    elif s.card == 'title':
        text_block(draw, s.headline, (48, 123, 984, 252), 112, bold=True, gap=0)
        text_block(draw, s.body, (52, 385, 976, 92), 43, bold=True)
        source = ROOT / 'docs/assets/filthy-casual-recliner.png'
        art = ImageOps.contain(Image.open(source).convert('RGB'), (1000, 1060))
        im.paste(art, ((W-art.width)//2, 540+(1060-art.height)//2))
        draw = ImageDraw.Draw(im)
        text_block(draw, 'LESS FUSS. MORE WASTELAND.', (52, 1670, 976, 100), 43, bold=True)
        stamp(draw, s.stamp)
    elif s.card == 'install':
        text_block(draw, s.headline, (48, 143, 984, 225), 96, bold=True, gap=0)
        for i, (head, body) in enumerate([
            ('ADD THE ZIP', 'Install the addon archive in MO2.'),
            ('ENABLE IT', 'Place it below the base TTR mod.'),
            ('GO PLAY', 'Run your usual TTR game entry.')]):
            y = 500 + i * 350
            draw.ellipse((48, y, 174, y+126), fill=RED)
            draw.text((80, y+31), f'{i+1:02}', font=font(44, True), fill=CREAM)
            text_block(draw, head, (210, y+8, 810, 84), 57, bold=True)
            text_block(draw, body, (210, y+101, 775, 150), 38, gap=13)
        text_block(draw, 'KEEP YOUR WORKING TTR SETUP.', (48, 1670, 980, 105), 41, bold=True)
        stamp(draw, s.stamp)
    else:
        text_block(draw, s.headline, (48, 190, 984, 240), 99, bold=True, gap=4)
        text_block(draw, s.body, (52, 585, 976, 210), 45, bold=True, gap=18)
        draw.line((50, 970, 1030, 970), fill=INK, width=4)
        for y, title, name in [(1060, 'BASE VR', 'KUPOkinz'), (1250, 'POOL TABLES OF THE MOJAVE', 'Luke2172'), (1440, 'ADDONS', 'Nikami')]:
            text_block(draw, title, (52, y, 976, 55), 29, color=RED, bold=True)
            text_block(draw, name, (52, y+57, 976, 80), 48, bold=True)
        stamp(draw, s.stamp)
    path = WORK / f'{s.key}-card.png'
    im.save(path)
    return path


def command(args):
    subprocess.run(args, check=True, timeout=900)


def render(shots):
    ffmpeg = shutil.which('ffmpeg')
    if not ffmpeg:
        raise RuntimeError('ffmpeg is required')
    base = [ffmpeg, '-hide_banner', '-loglevel', 'error', '-y']
    enc = ['-c:v', 'libx264', '-preset', 'veryfast', '-crf', '18', '-pix_fmt', 'yuv420p', '-threads', '4', '-an']
    clips = []
    for s in shots:
        card = caption_asset(s)
        args = base + ['-loop', '1', '-framerate', str(FPS), '-i', str(card)]
        if s.source:
            args += ['-ss', f'{s.start:.6f}', '-i', s.source]
            x, y, w, h = EYE
            # Preserve the full right eye. No in-eye crop, speed changes or invented frames.
            filters = f'[1:v]trim=duration={s.duration:.6f},setpts=PTS-STARTPTS,crop=1032:1104:1032:0,eq=gamma=1.22,scale={w}:{h},setsar=1[g];[0:v][g]overlay={x}:{y}:shortest=1[v]'
            args += ['-filter_complex_threads', '2', '-filter_complex', filters, '-map', '[v]']
        else:
            args += ['-vf', 'setsar=1']
        clip = WORK / f'{s.key}.mp4'
        command(args + ['-t', f'{s.duration:.6f}', '-r', str(FPS)] + enc + [str(clip)])
        clips.append(clip)
        print(f'Rendered {s.key}: {s.duration:.2f}s', flush=True)
    args = base.copy()
    for clip in clips:
        args += ['-i', str(clip)]
    filters, last, offset = [], '0:v', shots[0].duration - .4
    for i in range(1, len(clips)):
        target = f'cut{i}'
        transition = 'wipeleft' if i % 2 else 'wiperight'
        filters.append(f'[{last}][{i}:v]xfade=transition={transition}:duration=0.4:offset={offset:.6f}[{target}]')
        offset += shots[i].duration - .4
        last = target
    command(args + ['-filter_complex_threads', '2', '-filter_complex', ';'.join(filters), '-map', f'[{last}]'] + enc + ['-movflags', '+faststart', str(OUT)])


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--render', action='store_true', help='Encode only after fresh source captures are approved')
    ap.add_argument('--prepare-assets', action='store_true', help='Generate original portrait cards without encoding video')
    ap.add_argument('--fresh-after', help='ISO timestamp with timezone; reject older new-feature captures')
    ap.add_argument('--allow-missing-optionals', action='store_true', help='Partial draft plan only; never allowed when rendering')
    ap.add_argument('--cuts', type=Path, help='Optional JSON mapping shot key to {start,end} for reviewed source timings')
    args = ap.parse_args()
    if args.render and (not args.fresh_after or args.allow_missing_optionals):
        ap.error('Rendering requires --fresh-after and every planned pickup; no partial montage')
    fresh = None
    if args.fresh_after:
        fresh = datetime.fromisoformat(args.fresh_after.replace('Z', '+00:00'))
        if fresh.tzinfo is None:
            ap.error('--fresh-after must include a timezone')
        fresh = fresh.astimezone(timezone.utc)
    shots, omitted = make_shots(args.allow_missing_optionals)
    if args.cuts:
        cuts = read_json(args.cuts)
        unknown = set(cuts) - {s.key for s in shots}
        if unknown:
            raise ValueError(f'Unknown cut keys: {sorted(unknown)}')
        for s in shots:
            if s.key in cuts:
                s.start = float(cuts[s.key].get('start', s.start))
                s.end = float(cuts[s.key].get('end', s.end))
    # Every cut and wipe lands on an actual30fps frame boundary.
    # This prevents fractional-duration drift across the concatenated chapters.
    for s in shots:
        s.start = round(s.start * FPS) / FPS
        s.end = round(s.end * FPS) / FPS
    sources = validate(shots, fresh)
    WORK.mkdir(parents=True, exist_ok=True)
    total = sum(s.duration for s in shots) - .4 * (len(shots)-1)
    manifest = {'output': str(OUT), 'resolution': [W,H], 'fps': FPS, 'duration': total,
        'footageSpeed': 1, 'eye': 'complete right eye', 'gameplayColorGrade': 'global gamma 1.22 for phone visibility',
        'audio': False, 'frameInterpolation': False, 'transitionSeconds': .4,
        'cadencePolicy': {'maximumRepeatedFrameFraction': .005, 'maximumConsecutiveRepeatedFrames': 1, 'isolatedRepeatsPreserved': True, 'visualReviewRequired': True},
        'captureProvenance': 'Actual final composited game eyes with choreographed XR input',
        'freshAfter': fresh.isoformat() if fresh else None, 'omittedDraftShots': omitted,
        'sources': sources, 'shots': [dict(asdict(s), duration=s.duration, startFrame=round(s.start*FPS), endFrameExclusive=round(s.end*FPS)) for s in shots],
        'reviewRequired': ['Complete actions and readable hand contacts', 'Both source eyes', 'Phone-size caption readability', 'Full-speed final edit']}
    (WORK/'edit-plan.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(f'Planned {len(shots)} shots, {total:.2f}s. No source speed changes. Omitted: {omitted}')
    if args.prepare_assets:
        for s in shots:
            caption_asset(s)
        print(f'Prepared portrait caption cards: {WORK}')
    if not 95 <= total <= 125:
        print('NOTE: duration is outside the intended ~100–120s range; review cuts before rendering.')
        if args.render:
            raise ValueError('Review cuts to bring complete montage inside95–125s before rendering')
    if args.render:
        render(shots)
        manifest['renderedOutput'] = {'path': str(OUT), 'sha256': sha256(OUT), 'bytes': OUT.stat().st_size}
        (WORK/'edit-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
        print(f'Local review output: {OUT}')

if __name__ == '__main__':
    main()
