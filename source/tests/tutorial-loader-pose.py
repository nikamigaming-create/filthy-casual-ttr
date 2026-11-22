import json,math,sys
from pathlib import Path
r=Path(__file__).resolve().parents[1]
a=json.loads((r/'sim-session/addon-state.json').read_text(encoding='utf-8-sig'));s=json.loads((r/'sim-session/state.json').read_text(encoding='utf-8-sig'));request=json.loads((r/'work/tutorial-dev/loader-target.json').read_text(encoding='utf-8-sig'))
def mul(a,b):return [[sum(a[i][k]*b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
def transpose(a):return list(map(list,zip(*a)))
def norm(v):
 n=math.sqrt(sum(x*x for x in v));return [x/n for x in v]
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def rot(y,p,r):
 y,p,r=map(math.radians,(y,p,r));cy,sy,cp,sp,cr,sr=math.cos(y),math.sin(y),math.cos(p),math.sin(p),math.cos(r),math.sin(r)
 return mul(mul([[cy,0,sy],[0,1,0],[-sy,0,cy]],[[1,0,0],[0,cp,-sp],[0,sp,cp]]),[[cr,sr,0],[-sr,cr,0],[0,0,1]])
parts=[p for p in a['tutorial']['namedParts'] if p['node']==request.get('node','##SpeedLoader')]
held=parts[request['heldOccurrence']] if 'heldOccurrence' in request else min(parts,key=lambda p:sum((x-y)**2 for x,y in zip(p['tracking'],a['nativeGripTracking'][0])))
result=mul(mul(transpose(request['basis']),held['basis']),rot(*s['handL']['ypr']))
forward=[-row[2] for row in result];up=[row[1] for row in result];yaw=math.atan2(-forward[0],-forward[2]);pitch=math.asin(max(-1,min(1,forward[1])));right=norm(cross(forward,[0,1,0]));trueup=cross(right,forward);roll=math.atan2(dot(up,right),dot(up,trueup))
print(json.dumps({'angles':[math.degrees(v) for v in (yaw,pitch,roll)],'held':held['tracking']}))
