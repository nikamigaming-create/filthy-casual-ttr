"""Original pool technique study. No paid or downloaded motion data."""
import math
import numpy as np
import author_sunny_pose as a
DURATION=7.2
# Measured Sunny live root and hand scale; preserve native actor proportions.
ACTOR_SCALE=.95
CLOTH_HEIGHT=53.9
CONTACT=4.62
CUE_SLOPE=0.088
BALL_RADIUS=2.66666675
CONTACT_PULL=BALL_RADIUS/math.sqrt(1+CUE_SLOPE*CUE_SLOPE)
TUNING={'bridge_x': -0.4, 'bridge_z': 0.4, 'thumb_x': 0.0, 'thumb_z': 0.0, 'grip_x': 1.1632812499999998, 'grip_z': 0.13671875, 'grip_flex': -16.51171875, 'grip_thumb_x': -0.44999999999999996, 'grip_thumb_z': 1.691015625, 'bridge_flex1': -4.0, 'bridge_flex2': -1.0, 'bridge_flex3': -5.0, 'bridge_flex4': -8.5}
# Cue tip distance behind the ball centre. Delivery continues beyond impact.
KEYS=[(0,12),(.9,3.5),(1.4,3.5),(1.8,7),(2.15,3.5),(2.5,6),(2.85,3.5),(3.55,3.5),(4.1,9),(4.3,9),(4.62,CONTACT_PULL),(4.88,-5),(5.5,-5),(5.95,8),(7.2,8)]
def ease(x):
 x=float(np.clip(x,0,1));return x*x*x*(10+x*(-15+6*x))
def pull(t):
 for (t0,v0),(t1,v1) in zip(KEYS,KEYS[1:]):
  if t<=t1:
   u=float(np.clip((t-t0)/(t1-t0),0,1));h=t1-t0
   m0=-42 if t0==CONTACT else 0;m1=-42 if t1==CONTACT else 0
   if m0 or m1:return (2*u**3-3*u*u+1)*v0+(u**3-2*u*u+u)*h*m0+(-2*u**3+3*u*u)*v1+(u**3-u*u)*h*m1
   return v0+(v1-v0)*ease(u)
 return KEYS[-1][1]

def blend_rotation(r0,r1,w):
 q0=a.quat(r0);q1=a.quat(r1)
 if q0@q1<0:q1=-q1
 angle=math.acos(float(np.clip(q0@q1,-1,1)))
 q=(q0*(1-w)+q1*w) if angle<.001 else (q0*math.sin((1-w)*angle)+q1*math.sin(w*angle))/math.sin(angle)
 q=q/np.linalg.norm(q);x,y,z,v=q
 return np.array([[1-2*(y*y+z*z),2*(x*y-z*v),2*(x*z+y*v)],[2*(x*y+z*v),1-2*(x*x+z*z),2*(y*z-x*v)],[2*(x*z-y*v),2*(y*z+x*v),1-2*(x*x+y*y)]])

def pose(t,d=110,x=15):
 local={k:v.copy() for k,v in a.bind.items()}
 # Anchor the stance to the physical table, not the actor-scaled root.
 bias=np.array([x,d,CLOTH_HEIGHT])*(1/ACTOR_SCALE-1)
 setup=ease(t/1.2);recovery=ease((t-5.95)/1.25);w=setup*(1-recovery)
 yaw=math.radians(-45*w);hinge=math.radians(-80*w)
 body=a.world(local,'Bip01 NonAccum');body[:3,3]+=(np.array([x-30,d-80,-7])+bias)*w;body[2,3]-=math.sin(math.pi*w);body[:3,:3]=a.rotate([0,0,1],yaw)@body[:3,:3];a.set_world(local,'Bip01 NonAccum',body)
 spine=a.world(local,'Bip01 Spine');axis=a.rotate([0,0,1],yaw)@np.array([1,0,0]);spine[:3,:3]=a.rotate(axis,hinge)@spine[:3,:3];a.set_world(local,'Bip01 Spine',spine)
 for side,dx,dy,angle,delay in [('L',-30,53+(d-110),-15,0),('R',-3,5+(d-110),-55,.12)]:
  back=ease((t-5.95-delay)/(1.25-delay))
  dx+=x-15+bias[0];dy+=bias[1]
  step=ease((t-delay)/(1.2-delay))*(1-back)
  foot0=a.world(a.bind,'Bip01 '+side+' Foot')[:3,3].copy();foot=foot0+(np.array([dx,dy,foot0[2]])-foot0)*step
  foot[2]+=3*math.sin(math.pi*ease((t-delay)/(1.2-delay))) if delay<t<1.2 else 0
  if 5.95+delay<t<7.2:foot[2]+=3*math.sin(math.pi*back)
  a.ik(local,'Bip01 '+side+' Thigh','Bip01 '+side+' Calf','Bip01 '+side+' Foot',foot,[foot[0]+30*math.sin(math.radians(-angle*step)),foot[1]+30*math.cos(math.radians(angle*step)),35])
  f=a.world(local,'Bip01 '+side+' Foot');f[:3,:3]=a.rotate([0,0,1],math.radians(angle*step))@a.world(a.bind,'Bip01 '+side+' Foot')[:3,:3];a.set_world(local,'Bip01 '+side+' Foot',f)
 for side in ['L','R']:
  hand='Bip01 '+side+' Hand'
  # Wrist frames: bridge palm down, fingers towards the target; rear fingers
  # drape around the shaft. The thumb is authored separately from the fingers.
  rot=a.rotate([1,0,0],math.radians(0))@a.rotate([0,0,1],math.radians(20))@np.array([[0,0,-1],[1,0,0],[0,-1,0]]) if side=='L' else np.array([[0,-1,0],[0,0,1],[-1,0,0]])
  goal=np.array([x-1.6+TUNING["bridge_x"],d-22-6.8,56.0+TUNING["bridge_z"]]) if side=='L' else np.array([x+2.02+TUNING["grip_x"],d-88-pull(t)-.43,62.3+(88+pull(t))*CUE_SLOPE+TUNING["grip_z"]])
  standing=np.array([-14 if side=='L' else 14,4,74.])
  goal=standing*(1-w)+(goal+bias)*w
  pole=a.world(local,'Bip01 '+side+' UpperArm')[:3,3]+a.rotate([0,0,1],yaw)@np.array([0,-30,0] if side=='L' else [30,-30,20])
  a.ik(local,'Bip01 '+side+' UpperArm','Bip01 '+side+' Forearm',hand,goal,pole)
  if side=='L':rot=blend_rotation(a.world(local,'Bip01 L Forearm')[:3,:3]@a.bind[hand][:3,:3],rot,w*w)
  h=a.world(local,hand);h[:3,:3]=rot;a.set_world(local,hand,h)
  for finger in range(1,5):
   spread=[-9,-2,5,13][finger-1] if side=='L' else [-2,0,1,3][finger-1]
   flex=([TUNING["bridge_flex"+str(finger)],20,10] if side=='L' else [[48,70,40],[52,72,42],[55,74,42],[58,74,37]][finger-1])
   for j,suffix in enumerate(['','1','2']):
    name=f'Bip01 {side} Finger{finger}{suffix}'
    if j==0:local[name][:3,:3]=a.rotate([0,1,0],math.radians(spread*(w if side=="L" else 1)))@local[name][:3,:3]
    local[name][:3,:3]=local[name][:3,:3]@a.rotate([0,0,1],math.radians((flex[j]+(TUNING["grip_flex"] if side=="R" else 0))*(w if side=="L" else 1)))
  # Open bridge: thumb raised beside index instead of clenched across palm.
  for j,suffix in enumerate(['','1','2']):
   name=f'Bip01 {side} Thumb1{suffix}'
   local[name][:3,:3]=local[name][:3,:3]@a.rotate([0,0,1],math.radians(([5,5,8] if side=='L' else [-15,-22,-20])[j]*(w if side=='L' else 1)))
 # Anatomical bridge and loose rear grip; blend without a threshold snap.
 gy=d-88-pull(t);gz=(88+pull(t))*CUE_SLOPE
 targets=[('L','Thumb1','Thumb11',[x+1.8,d-24.3,56.1]),
          ('L','Thumb11','Thumb12',[x+.8,d-21.6,56.3]),
          ('R','Thumb1','Thumb11',[x-2,gy+3,58.0+gz]),
          ('R','Thumb11','Thumb12',[x-1.5,gy+1.8,55.2+gz])]
 for side,parent,child,target in targets:
  target=np.array(target,dtype=float)
  if side=='L':
   target+=np.array([TUNING['thumb_x'],0,TUNING['thumb_z']])+bias
   # Keep the thumb's target anatomical while the approaching wrist rotates.
   bridge=np.array([x-1.6+TUNING['bridge_x'],d-28.8,56+TUNING['bridge_z']])+bias
   bridge_rot=a.rotate([0,0,1],math.radians(20))@np.array([[0,0,-1],[1,0,0],[0,-1,0]])
   wrist=a.world(local,'Bip01 L Hand')
   target=wrist[:3,3]+wrist[:3,:3]@bridge_rot.T@(target-bridge)
  else:
   work=np.array([x+2.02+TUNING['grip_x'],gy-.43,62.3+gz+TUNING['grip_z']])
   target+=np.array([TUNING['grip_thumb_x'],0,TUNING['grip_thumb_z']])+a.world(local,'Bip01 R Hand')[:3,3]-work
  base='Bip01 '+side+' ';current=a.world(local,base+child)[:3,3]
  weight=w if side=='L' else 1
  a.swing(local,base+parent,base+child,current*(1-weight)+np.array(target)*weight)
 head=a.world(local,'Bip01 Head');forward=a.unit(np.array([x,d,56])+bias-head[:3,3]);right=a.unit(np.cross(forward,[0,0,1]));up=np.cross(right,forward)
 head[:3,:3]=np.column_stack([up,forward,-right]);a.set_world(local,'Bip01 Head',head)
 return local
def cue_tip(t,d=110,x=15,local=None):
 local=pose(t,d,x) if local is None else local
 wrist=a.world(local,'Bip01 R Hand')[:3,3]
 return ACTOR_SCALE*(wrist+np.array([-2.02-TUNING['grip_x'],88.43,-6.3-TUNING['grip_z']-88*CUE_SLOPE]))

if __name__=='__main__':
 for d,x in [(110,15),(120,15)]:
  p=pose(4.62,d,x)
  for n in ['Head','L UpperArm','L Forearm','L Hand','R UpperArm','R Forearm','R Hand']:
   print(d,n,np.round(a.world(p,'Bip01 '+n)[:3,3],2))
