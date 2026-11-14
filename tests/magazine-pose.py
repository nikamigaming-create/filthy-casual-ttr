import math,json,sys
from pathlib import Path
r=Path(__file__).resolve().parents[1]
a=json.loads((r/'sim-session/addon-state.json').read_text(encoding='utf-8-sig'))
state=json.loads((r/'sim-session/state.json').read_text(encoding='utf-8-sig'))
def mul(a,b):return [[sum(a[i][k]*b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
def transpose(a):return list(map(list,zip(*a)))
def norm(v):
 l=math.sqrt(sum(x*x for x in v));return [x/l for x in v]
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def rot(y,p,r):
 y,p,r=map(math.radians,(y,p,r));cy,sy,cp,sp,cr,sr=math.cos(y),math.sin(y),math.cos(p),math.sin(p),math.cos(r),math.sin(r)
 return mul(mul([[cy,0,sy],[0,1,0],[-sy,0,cy]],[[1,0,0],[0,cp,-sp],[0,sp,cp]]),[[cr,sr,0],[-sr,cr,0],[0,0,1]])
current=rot(*state['handL']['ypr'])
if sys.argv[1]=='align':
 target=transpose(a['insertion']['restBasis']);held=transpose(a['insertion']['heldBasis']);delta=mul(target,transpose(held))
else:
 part=next(p for p in a['physicalParts'] if p['ref']==int(sys.argv[2]));origin=a['gloves'][0]['origin'];start=norm(a['gloves'][0]['forward']);end=norm([x-y for x,y in zip(part['tracking'],origin)])
 v=cross(start,end);c=dot(start,end)
 if c<-.999:raise RuntimeError('aim opposite; turn toward object first')
 k=[[0,-v[2],v[1]],[v[2],0,-v[0]],[-v[1],v[0],0]];k2=mul(k,k)
 delta=[[(1 if i==j else 0)+k[i][j]+k2[i][j]/(1+c) for j in range(3)] for i in range(3)]
result=mul(delta,current);forward=[-row[2] for row in result];up=[row[1] for row in result]
yaw=math.atan2(-forward[0],-forward[2]);pitch=math.asin(max(-1,min(1,forward[1])));right=norm(cross(forward,[0,1,0]));trueup=cross(right,forward);roll=math.atan2(dot(up,right),dot(up,trueup))
print(json.dumps([math.degrees(x) for x in (yaw,pitch,roll)]))
