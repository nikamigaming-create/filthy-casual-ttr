"""Keep comic captions whole while the gameplay window wipes between takes."""
from pathlib import Path
import json,subprocess,shutil
r=Path(__file__).resolve().parents[1];w=r/'outputs/physical-parts/mobile-edit'
m=json.loads((w/'edit-manifest.json').read_text());shots=m['shots'];ff=shutil.which('ffmpeg')
a=[ff,'-v','error','-y']
for s in shots:a+=['-i',str(w/(s['key']+'.mp4'))]
f=[];cards=[];n=len(shots)
for i,s in enumerate(shots):
 d=s['end']-s['start'];begin=0 if i==0 else .2;end=d if i==n-1 else d-.2
 f += [f'[{i}:v]split=2[c{i}][g{i}]',f'[c{i}]trim=start={begin}:end={end},setpts=PTS-STARTPTS[c{i}t]',f'[g{i}]crop=1000:1070:40:390,setpts=PTS-STARTPTS[g{i}t]']
 cards.append(f'[c{i}t]')
f.append(''.join(cards)+f'concat=n={n}:v=1:a=0[cards]')
last='g0t';offset=shots[0]['end']-shots[0]['start']-.4
for i in range(1,n):
 target=f'wipe{i}';trans='wipeleft' if i%2 else 'wiperight'
 f.append(f'[{last}][g{i}t]xfade=transition={trans}:duration=0.4:offset={offset:.6f}[{target}]')
 offset+=shots[i]['end']-shots[i]['start']-.4;last=target
f.append(f'[cards][{last}]overlay=40:390:shortest=1[out]')
out=r/'outputs/Filthy-Casual-Physical-Parts-Mobile.mp4'
a+=['-filter_complex_threads','2','-filter_complex',';'.join(f),'-map','[out]','-an','-c:v','libx264','-preset','veryfast','-crf','18','-pix_fmt','yuv420p','-threads','4','-movflags','+faststart',str(out)]
subprocess.run(a,check=True)
import hashlib
m['outputSha256']=hashlib.sha256(out.read_bytes()).hexdigest();m['transitions']='Gameplay-only0.4secondwipes; captions switch intact at midpoint.'
(w/'edit-manifest.json').write_text(json.dumps(m,indent=2));print(out)