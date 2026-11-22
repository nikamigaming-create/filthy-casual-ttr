"""Solve a real free-hand index pointing pose from native bone observations.
Only emits a pose for the isolated XR simulator; never edits game/input state.
"""
import json,math,sys
from pathlib import Path
r=Path(__file__).resolve().parents[1]
a=json.loads((r/'sim-session/addon-state.json').read_text(encoding='utf-8-sig'))
s=json.loads((r/'sim-session/state.json').read_text(encoding='utf-8-sig'))
target=next(p['tracking'] for p in a['tutorial']['parts']+a['tutorial']['namedParts'] if p['node']==sys.argv[1])
def mul(a,b):return [[sum(a[i][k]*b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def norm(v):
 n=math.sqrt(dot(v,v))
 if n<1e-6:raise RuntimeError('No valid native finger direction')
 return [x/n for x in v]
def rot(y,p,r):
 y,p,r=map(math.radians,(y,p,r));cy,sy,cp,sp,cr,sr=math.cos(y),math.sin(y),math.cos(p),math.sin(p),math.cos(r),math.sin(r)
 return mul(mul([[cy,0,sy],[0,1,0],[-sy,0,cy]],[[1,0,0],[0,cp,-sp],[0,sp,cp]]),[[cr,sr,0],[-sr,cr,0],[0,0,1]])
base,tip=a['tutorial']['pointer'];offset=[-.20,.02,.16]
desired=[x+y for x,y in zip(target,offset)]
start=norm([x-y for x,y in zip(tip,base)]);end=norm([-v for v in offset]);v=cross(start,end);c=dot(start,end)
if c<-.95:raise RuntimeError('Pointing requires a different relaxed starting pose')
k=[[0,-v[2],v[1]],[v[2],0,-v[0]],[-v[1],v[0],0]];k2=mul(k,k)
delta=[[(1 if i==j else 0)+k[i][j]+k2[i][j]/(1+c) for j in range(3)] for i in range(3)]
result=mul(delta,rot(*s['handL']['ypr']));forward=[-row[2] for row in result];up=[row[1] for row in result]
yaw=math.atan2(-forward[0],-forward[2]);pitch=math.asin(max(-1,min(1,forward[1])));right=norm(cross(forward,[0,1,0]));trueup=cross(right,forward);roll=math.atan2(dot(up,right),dot(up,trueup))
pos=[p+d-t for p,d,t in zip(s['handL']['pos'],desired,tip)]
print(json.dumps({'position':pos,'angles':[math.degrees(x) for x in (yaw,pitch,roll)],'target':target,'tip':tip,'gap':math.sqrt(dot(offset,offset))}))
