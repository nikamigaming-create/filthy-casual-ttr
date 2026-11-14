"""Bounded full-duration decode/cadence check and sequential both-eye review sheets."""
from pathlib import Path
import sys,json,cv2,numpy as np
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1];out=root/'outputs/vehicle-showcase/review';out.mkdir(exist_ok=True)
for name in sys.argv[1:]:
 path=root/'outputs/casual-guide'/(name+'-stereo.mp4');cap=cv2.VideoCapture(str(path));fps=cap.get(cv2.CAP_PROP_FPS);count=0;black=[];diff=[];previous=None;thumbs=[[],[]]
 while True:
  ok,frame=cap.read()
  if not ok:break
  small=cv2.resize(frame,(320,172));gray=cv2.cvtColor(small,cv2.COLOR_BGR2GRAY);fraction=float(np.mean(gray>12))
  if fraction<.02:black.append(count)
  if previous is not None:diff.append(float(np.mean(cv2.absdiff(gray,previous))))
  previous=gray
  if count%round(fps*4)==0:
   for eye in range(2):
    half=frame[:,eye*(frame.shape[1]//2):(eye+1)*(frame.shape[1]//2)];im=Image.fromarray(cv2.cvtColor(half,cv2.COLOR_BGR2RGB));im.thumbnail((340,364));tile=Image.new('RGB',(340,388),(15,22,20));tile.paste(im,((340-im.width)//2,22));ImageDraw.Draw(tile).text((6,4),f'{count/fps:.2f}s / '+('L' if eye==0 else 'R'),fill='white');thumbs[eye].append(tile)
  count+=1
 cap.release();meta=json.loads(path.with_suffix('.json').read_text());assert count==meta['encodedFrames']
 for eye in range(2):
  for start in range(0,len(thumbs[eye]),12):
   sheet=Image.new('RGB',(1020,1552),(15,22,20))
   for j,tile in enumerate(thumbs[eye][start:start+12]):sheet.paste(tile,((j%3)*340,(j//3)*388))
   sheet.save(out/(name+f'-{eye}-{start//12}.jpg'),quality=88)
 result=dict(pid=meta['pid'],frames=count,seconds=count/fps,repeated_frames=meta['repeatedFrames'],black_frames=black,frame_difference_p99=float(np.percentile(diff,99)) if diff else 0,max_frame_difference=max(diff,default=0),sampling_seconds=4)
 (out/(name+'-check.json')).write_text(json.dumps(result,indent=2));print(name,result['frames'],result['seconds'],'black',len(black),'repeats',result['repeated_frames'])
