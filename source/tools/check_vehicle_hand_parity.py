"""Compare native raw/contact/visible-wrist motion in the recorded carrier frame."""
import argparse,json
from pathlib import Path
import numpy as np
ap=argparse.ArgumentParser();ap.add_argument('name');ap.add_argument('trace');ap.add_argument('--expect-failure',action='store_true');a=ap.parse_args()
root=Path(__file__).resolve().parents[1];g=root/'outputs/casual-guide'
meta=json.loads((g/(a.name+'-stereo.json')).read_text());events=json.loads((g/(a.name+'-timeline.json')).read_text(encoding='utf-8-sig'))
rows=[];lines=Path(a.trace).read_text().splitlines()
for i,l in enumerate(lines):
 try: rows.append(json.loads(l))
 except json.JSONDecodeError:
  assert i==len(lines)-1,'Malformed interior trace row'
assert rows and all(r['pid']==meta['pid'] for r in rows)
start=meta['frames'][0]['sourceQpc'];frequency=meta['frames'][0]['qpcFrequency'];assert frequency>0
checks=[]
for event,length in [('Fixed head and hands stationary',3),('Climb fixed hands',2.5),('Forward fixed hands',4.5),('Reverse fixed hands',2.5),('Turn with fixed hands',1.8)]:
 t=next(e['seconds'] for e in events if e['action']==event)+.8
 phase=[r for r in rows if t<(r['qpc']-start)/frequency<t+length];assert len(phase)>=30
 for h in range(2):
  def values(key):return np.array([r['hands'][h][key] for r in phase])
  raw=values('rawWorld');target=values('target');wrist=np.array([r['hands'][h]['bones'][0] for r in phase]);correction=values('correction')
  result=dict(phase=event,hand=h,samples=len(phase),rawSpan=float(np.linalg.norm(np.ptp(raw,axis=0))),targetSpan=float(np.linalg.norm(np.ptp(target,axis=0))),wristSpan=float(np.linalg.norm(np.ptp(wrist,axis=0))),maxContactCorrection=float(np.max(np.linalg.norm(correction,axis=1))))
  result['pass']=result['rawSpan']<.1 and result['targetSpan']<.15 and result['wristSpan']<.25 and result['maxContactCorrection']<.15
  checks.append(result)
result={'pid':meta['pid'],'units':'Fallout world units;70units/metre in this fixture','source':'native end-publication trace joined to final-eye sourceQpc','checks':checks,'allPass':all(c['pass'] for c in checks),'carries':rows[-1].get('contactCarries'),'misses':rows[-1].get('contactMisses'),'reseeds':rows[-1].get('contactReseeds')}
(root/'work/vertibird/classic'/(a.name+'-result.json')).write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
assert result['allPass']!=a.expect_failure,'Unexpected parity gate result'
if not a.expect_failure:assert result['carries']>100 and result['misses']==0 and result['reseeds']==1
