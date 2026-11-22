"""Record actual final-eye frames at wall-clock cadence (Windows, private sim).

Start after `video on`. A sidecar records the source frame and capture age for
every encoded frame, including repeats; it never invents an animation cadence.
No desktop screenshots, input injection, or game assets are used.
"""
import argparse
import json
import mmap
import pathlib
import shutil
import struct
import subprocess
import time
import sys, threading, wave

parser = argparse.ArgumentParser()
parser.add_argument('--pid', type=int, required=True)
parser.add_argument('--out', type=pathlib.Path, required=True)
parser.add_argument('--seconds', type=float, default=30)
parser.add_argument('--audio', action='store_true', help='Record the default WASAPI output loopback with this take.')
parser.add_argument('--fps', type=int, default=30)
parser.add_argument('--stop-file', type=pathlib.Path, help='Finish cleanly and save metadata when this file appears.')
args = parser.parse_args()
if not 0 < args.seconds <= 180 or not 1 <= args.fps <= 60:
    raise SystemExit('Bounded recording: 1-180 seconds, 1-60 fps')
args.out.parent.mkdir(parents=True, exist_ok=True)
header = struct.Struct('<8I3Q2I')
shared = mmap.mmap(-1, 32*1024*1024,
                   tagname=f'Local\\TTWVR_Seated_FinalEyes_{args.pid}')

def sample():
    for _ in range(15):
        h = header.unpack(shared[:64])
        if h[0] & 1 or h[1] != 0x53435631 or h[6] != args.pid:
            time.sleep(.001)
            continue
        width, height, stride = h[3:6]
        if not (0 < width <= 4096 and 0 < height <= 4096 and stride == width*4):
            raise RuntimeError('Invalid video header')
        pixels = shared[64:64+stride*height]
        if struct.unpack('<I', shared[:4])[0] == h[0]:
            return h, pixels
    return None

deadline = time.perf_counter()+10
first = None
while first is None and time.perf_counter() < deadline:
    first = sample()
if first is None:
    raise SystemExit('No live final-eye video transport; enable video on')
width, height = first[0][3:5]
ffmpeg = shutil.which('ffmpeg')
if not ffmpeg:
    raise SystemExit('ffmpeg required')
process = subprocess.Popen([
    ffmpeg, '-hide_banner', '-loglevel', 'warning', '-y', '-f', 'rawvideo',
    '-pixel_format', 'bgra', '-video_size', f'{width}x{height}', '-framerate',
    str(args.fps), '-i', 'pipe:0', '-an', '-c:v', 'libx264', '-preset', 'veryfast',
    '-crf', '18', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(args.out)
], stdin=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
records = []
previous = first
audio_stop=threading.Event();audio_thread=None;audio_start=None;audio_error=[]
if args.audio:
    sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'work/audio-capture'))
    import pyaudiowpatch as pa
    audio_api=pa.PyAudio();device=audio_api.get_default_wasapi_loopback()
    audio_rate=int(device['defaultSampleRate']);audio_channels=device['maxInputChannels']
    audio_stream=audio_api.open(format=pa.paInt16,channels=audio_channels,rate=audio_rate,input=True,input_device_index=device['index'],frames_per_buffer=512,start=False)
    audio_path=args.out.with_suffix('.wav')
    def record_audio():
        try:
            with wave.open(str(audio_path),'wb') as w:
                w.setnchannels(audio_channels);w.setsampwidth(2);w.setframerate(audio_rate)
                while not audio_stop.is_set():w.writeframes(audio_stream.read(512,exception_on_overflow=False))
        except Exception as e:audio_error.append(str(e))
    audio_stream.start_stream();audio_start=time.perf_counter()
    audio_thread=threading.Thread(target=record_audio);audio_thread.start()
start = time.perf_counter()
args.out.with_suffix('.ready.json').write_text(json.dumps(dict(startQpcSeconds=start,audioStartQpcSeconds=audio_start,pid=args.pid)))
try:
    for index in range(round(args.seconds*args.fps)):
        if args.stop_file and args.stop_file.exists():
            break
        wait = start+index/args.fps-time.perf_counter()
        if wait > 0:
            time.sleep(wait)
        current = sample()
        if current is None:
            current = previous
        h, pixels = current
        now = time.perf_counter()
        records.append(dict(frame=index, wallSeconds=now-start, sourceFrame=h[8],
                            sourceQpc=h[9], qpcFrequency=h[10], layers=h[11],
                            repeated=index > 0 and h[8] == previous[0][8],
                            readRetryExhausted=current is previous))
        process.stdin.write(pixels)
        previous = current
finally:
    if audio_thread:
        audio_stop.set();audio_thread.join(timeout=5);audio_stream.stop_stream();audio_stream.close();audio_api.terminate()
    process.stdin.close()
    code = process.wait(timeout=30)
    shared.close()
    elapsed = time.perf_counter()-start
    meta = dict(pid=args.pid, source='private simulator final composited stereo',
                audio=args.audio,audioStartOffsetSeconds=(audio_start-start if audio_start else None),audioErrors=audio_error, width=width, height=height, fps=args.fps,
                encodedFrames=len(records), wallSeconds=elapsed,
                repeatedFrames=sum(r['repeated'] for r in records),
                ffmpegExit=code, frames=records)
    args.out.with_suffix('.json').write_text(json.dumps(meta, indent=2))
if code:
    raise SystemExit(code)
print(json.dumps({k:v for k,v in meta.items() if k != 'frames'}))
