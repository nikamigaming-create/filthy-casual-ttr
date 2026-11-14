"""Reject render freezes even when native grab ownership and physics are correct."""
import argparse,json,re
from pathlib import Path
def analyze(path):
 text=Path(path).read_text(errors='replace')
 result=[]
 for section in text.split('EMPTY BOTTLE staged native pickup')[1:]:
  base=re.search(r'base=([0-9A-F]+)',section).group(1)
  if 'Aid grip: native control resumed' not in section:continue
  section=section.split('Aid grip: native control resumed',1)[1]
  rows=re.findall(r'\[(\d+)\] BOTTLE POSE ref=([0-9A-F]+) held=0 root=([-0-9.]+) ([-0-9.]+) ([-0-9.]+).*?hk=([-0-9.]+) ([-0-9.]+) ([-0-9.]+)',section)
  stall=0;worst=0;moving=0;visible=0
  for a,b in zip(rows,rows[1:]):
   dt=int(b[0])-int(a[0]); dr=sum((float(b[i])-float(a[i]))**2 for i in (2,3,4)); dh=sum((float(b[i])-float(a[i]))**2 for i in (5,6,7))
   if 0<dt<100 and dh>.0001:
    moving+=1; visible+=dr>.000001
    stall=stall+dt if dr<.000001 else 0;worst=max(worst,stall)
   else:stall=0
  result.append(dict(base=base,samples=len(rows),renderedPositions=len(set(x[2:5] for x in rows)),physicsPositions=len(set(x[5:8] for x in rows)),movingPhysicsIntervals=moving,visibleMovingIntervals=visible,longestRenderStallMs=worst,passesVisibleFlight=moving>8 and visible/max(moving,1)>.95 and worst<100))
 return result

if __name__ == '__main__':
 parser=argparse.ArgumentParser();parser.add_argument('log');parser.add_argument('--expect',choices=('pass','fail'),default='pass');parser.add_argument('--families',type=int,default=4);args=parser.parse_args()
 result=analyze(args.log);passed=len(result)==args.families and all(x['passesVisibleFlight'] for x in result)
 print(json.dumps(result,indent=2))
 if passed != (args.expect=='pass'):raise SystemExit('Unexpected rendered bottle trajectory verdict')
