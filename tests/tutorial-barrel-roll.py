"""Rotate a held gun about its observed barrel axis, not controller Euler Z.
Capture authoring only; reads native final frames and emits one simulator pose.
"""
import json,math,sys
from pathlib import Path
r=Path(__file__).resolve().parents[1]
a=json.loads((r/'sim-session/addon-state.json').read_text(encoding='utf-8-sig'))
s=json.loads((r/'sim-session/state.json').read_text(encoding='utf-8-sig'))
def mul(a,b):return [[sum(a[i][k]*b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def norm(v):
 n=math.sqrt(dot(v,v))
 if n<1e-6:raise ValueError('Missing native basis')
 return [x/n for x in v]
def rot(y,p,r):
 y,p,r=map(math.radians,(y,p,r));cy,sy,cp,sp,cr,sr=math.cos(y),math.sin(y),math.cos(p),math.sin(p),math.cos(r),math.sin(r)
 return mul(mul([[cy,0,sy],[0,1,0],[-sy,0,cy]],[[1,0,0],[0,cp,-sp],[0,sp,cp]]),[[cr,sr,0],[-sr,cr,0],[0,0,1]])
node=next(p for p in a['tutorial']['namedParts'] if p['node']=='##Shells')
# Local X is the authored barrel axis for this verified shotgun family.
axis=norm(node['basis'][0]);angle=math.radians(float(sys.argv[1]));c=math.cos(angle);sn=math.sin(angle)
k=[[0,-axis[2],axis[1]],[axis[2],0,-axis[0]],[-axis[1],axis[0],0]]
delta=[[(c if i==j else 0)+(1-c)*axis[i]*axis[j]+sn*k[i][j] for j in range(3)] for i in range(3)]
assert sum((sum(delta[i][j]*axis[j] for j in range(3))-axis[i])**2 for i in range(3))<1e-10
result=mul(delta,rot(*s['handR']['ypr']))
forward=[-row[2] for row in result];up=[row[1] for row in result]
yaw=math.atan2(-forward[0],-forward[2]);pitch=math.asin(max(-1,min(1,forward[1])))
right=norm(cross(forward,[0,1,0]));trueup=cross(right,forward);roll=math.atan2(dot(up,right),dot(up,trueup))
print(json.dumps({'angles':[math.degrees(x) for x in (yaw,pitch,roll)],'axis':axis,'degrees':float(sys.argv[1])}))
