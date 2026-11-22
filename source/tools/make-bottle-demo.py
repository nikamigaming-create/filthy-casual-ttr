"""Assemble reviewed real-time bottle footage with its original WASAPI audio."""
from pathlib import Path
import json,subprocess,shutil,wave,math
import numpy as np
root=Path(__file__).resolve().parents[1];src=root/'outputs/bottle-demo';out=root/'outputs/Filthy-Casual-Bottles-With-Sound.mp4'
shots=[('01-soft-drop',6.5,'LIGHT DROP - INTACT'),('02-floor-smash',7.5,'HARD IMPACT - SHATTER'),('03-sign-smash',5.5,'SIGN IMPACT'),('04-sign-smash',5.5,'SIGN IMPACT'),('05-sign-smash',16,'GLASS FADES AWAY')]
peak=0;manifest=[]
for name,end,label in shots:
 p=src/(name+'-stereo.mp4');m=json.loads(p.with_suffix('.json').read_text())
 assert m['ffmpegExit']==0 and m['audio'] and not m['audioErrors'] and m['repeatedFrames']/m['encodedFrames']<.02
 assert m['encodedFrames']/m['fps']>=end
 with wave.open(str(p.with_suffix('.wav'))) as w:
  assert w.getnframes()/w.getframerate()>=end
  a=np.frombuffer(w.readframes(round(end*w.getframerate())),dtype='<i2')/32768;peak=max(peak,float(abs(a).max()))
 manifest.append(dict(name=name,pid=m['pid'],seconds=end,label=label,audioOffset=m['audioStartOffsetSeconds']))
assert peak>1e-5
gain=min(.85/peak,100);cmd=[shutil.which('ffmpeg'),'-hide_banner','-loglevel','warning','-y'];filters=[]
for i,(name,end,label) in enumerate(shots):
 p=src/(name+'-stereo.mp4');cmd+=['-i',str(p),'-i',str(p.with_suffix('.wav'))]
 filters.append(f"[{2*i}:v]trim=duration={end},setpts=PTS-STARTPTS,crop=iw/2:ih:0:0,scale=720:770,setsar=1,drawtext=fontfile='C\\:/Windows/Fonts/consolab.ttf':text='{label}':fontsize=24:fontcolor=0xFFD47C:x=20:y=24:box=1:boxcolor=0x102018@0.8:boxborderw=10[v{i}]")
 offset=max(0,-manifest[i]['audioOffset'])
 filters.append(f'[{2*i+1}:a]atrim=start={offset}:duration={end},asetpts=PTS-STARTPTS,volume={gain},alimiter=limit=0.95:level=false[a{i}]')
filters.append(''.join(f'[v{i}][a{i}]' for i in range(len(shots)))+f'concat=n={len(shots)}:v=1:a=1[v][a]')
cmd+=['-filter_complex_threads','2','-filter_complex',';'.join(filters),'-map','[v]','-map','[a]','-c:v','libx264','-preset','fast','-crf','20','-threads','4','-pix_fmt','yuv420p','-c:a','aac','-b:a','160k','-movflags','+faststart',str(out)]
subprocess.run(cmd,check=True)
(src/'edit-manifest.json').write_text(json.dumps(dict(output=str(out),shots=manifest,audio='unaltered game loopback, common gain only',gain=gain,peakBefore=peak,sourceSpeed=1),indent=2))
print(out)
