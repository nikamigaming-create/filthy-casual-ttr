"""Author the pool pose against the user's owned FNV skeleton, locally.

Retail skeleton/reference files stay in ignored work/. Output animations are
newly generated joint keys, not copies of retail animation tracks.
"""
from pathlib import Path
import struct,zlib,time,os
import math
import json
from functools import lru_cache
from concurrent.futures import ProcessPoolExecutor
import numpy as np
time.clock=time.perf_counter
from pyffi.formats.nif import NifFormat as N
ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'work/pool-animation'
WORK.mkdir(parents=True,exist_ok=True)
class BSA:
    def __init__(self,path):
        self.f=open(path,'rb')
        magic,version,offset,self.flags,nfolders,nfiles,foldersize,namesize,fileflags=struct.unpack('<4s8I',self.f.read(36))
        assert magic==b'BSA\0' and version==104
        self.f.seek(offset)
        folders=[struct.unpack('<QII',self.f.read(16)) for _ in range(nfolders)]
        entries=[]
        for _,n,_ in folders:
            size=self.f.read(1)[0];folder=self.f.read(size).rstrip(b'\0').decode()
            entries.extend((folder,s,o) for _,s,o in [struct.unpack('<QII',self.f.read(16)) for _ in range(n)])
        names=self.f.read(namesize).split(b'\0')
        self.entries={(folder+'\\'+name.decode()).lower():(size,offset) for (folder,size,offset),name in zip(entries,names)}
    def read(self,path):
        size,offset=self.entries[path.lower()];self.f.seek(offset)
        data=self.f.read(size&0x3fffffff)
        if self.flags&0x100:data=data[data[0]+1:]
        if bool(self.flags&4)^bool(size&0x40000000):
            length=struct.unpack_from('<I',data)[0];data=zlib.decompress(data[4:]);assert len(data)==length
        return data
archive=BSA(Path(os.environ['FNV_GAME_DIR'])/'Data/Fallout - Meshes.bsa')
path=WORK/'skeleton.nif'
owned=archive.read(r'meshes\characters\_male\skeleton.nif')
if not path.exists() or path.read_bytes()!=owned:path.write_bytes(owned)
skel=N.Data()
with path.open('rb') as f:skel.read(f)
nodes={n.name.decode():n for n in skel.roots[0].tree() if isinstance(n,N.NiNode)}
parents={}
for name,n in nodes.items():
    for child in n.children:
        if isinstance(child,N.NiNode):parents[child.name.decode()]=name
bind={name:np.asarray(n.get_transform().as_list(),dtype=float).T for name,n in nodes.items()}
def unit(v):return v/max(1e-9,np.linalg.norm(v))
def rotate(axis,angle):
    x,y,z=unit(axis);c=math.cos(angle);s=math.sin(angle);d=1-c
    return np.array([[c+x*x*d,x*y*d-z*s,x*z*d+y*s],[y*x*d+z*s,c+y*y*d,y*z*d-x*s],[z*x*d-y*s,z*y*d+x*s,c+z*z*d]])
def from_to(a,b):
    a=unit(a);b=unit(b);axis=np.cross(a,b);s=np.linalg.norm(axis);c=np.clip(a@b,-1,1)
    if s<1e-8:return np.eye(3) if c>0 else rotate(np.cross(a,[0,0,1] if abs(a[2])<.9 else [1,0,0]),math.pi)
    return rotate(axis,math.atan2(s,c))
def world(local,name):
    return world(local,parents[name])@local[name] if name in parents else local[name].copy()
def set_world(local,name,m):
    local[name]=np.linalg.inv(world(local,parents[name]))@m if name in parents else m.copy()
def swing(local,bone,child,target):
    m=world(local,bone);tip=world(local,child)[:3,3]
    m[:3,:3]=from_to(tip-m[:3,3],target-m[:3,3])@m[:3,:3];set_world(local,bone,m)
def ik(local,a,b,c,target,pole):
    shoulder=world(local,a)[:3,3];elbow=world(local,b)[:3,3];hand=world(local,c)[:3,3]
    l1=np.linalg.norm(elbow-shoulder);l2=np.linalg.norm(hand-elbow)
    delta=np.asarray(target)-shoulder;direction=unit(delta);distance=np.clip(np.linalg.norm(delta),abs(l1-l2)+.01,l1+l2-.05)
    bend=unit(np.asarray(pole)-shoulder-direction*((np.asarray(pole)-shoulder)@direction))
    along=(l1*l1-l2*l2+distance*distance)/(2*distance)
    mid=shoulder+direction*along+bend*math.sqrt(max(0,l1*l1-along*along))
    swing(local,a,b,mid);swing(local,b,c,shoulder+direction*distance)
@lru_cache(maxsize=64)
def body_pose(distance,lateral):
    local={k:v.copy() for k,v in bind.items()}
    # Root motion stays zero. Bend at the hips with a split stance and both
    # feet solved back onto the floor. These are authored pool poses.
    body=world(local,'Bip01 NonAccum');body[:3,3]+=np.array([lateral*.75,20+np.clip((distance-110)*.35,0,22),-22]);set_world(local,'Bip01 NonAccum',body)
    spine=world(local,'Bip01 Spine');spine[:3,:3]=rotate([1,0,0],math.radians(-38-np.clip((distance-110)*.2,0,10)))@spine[:3,:3];set_world(local,'Bip01 Spine',spine)
    for side,forward in [('L',16),('R',-10)]:
        foot=world(bind,'Bip01 '+side+' Foot')[:3,3].copy();foot[1]+=forward
        ik(local,'Bip01 '+side+' Thigh','Bip01 '+side+' Calf','Bip01 '+side+' Foot',foot,[(-12 if side=='L' else 12),70,30])
        foot_pose=world(local,'Bip01 '+side+' Foot');foot_pose[:3,:3]=world(bind,'Bip01 '+side+' Foot')[:3,:3];set_world(local,'Bip01 '+side+' Foot',foot_pose)
    return local

def pose(age,distance=110,lateral=5):
    local={k:v.copy() for k,v in body_pose(distance,lateral).items()}
    pull=10+6*math.sin(age*2*math.pi) if age<2 else 16*(1-np.clip((age-2)/.5,0,1))
    for side,grip in [('L',min(80,max(5,distance-55))),('R',min(92,distance-25))]:
        # The bridge hand stays on the cloth; the rear hand slides the cue.
        goal=np.array([lateral,distance-grip-(pull if side=='R' else 0),56.])
        wrist_name='Bip01 '+side+' Hand'
        rotation=np.array([[0,1,0],[1,0,0],[0,0,-1]]) if side=='L' else np.array([[0,1,0],[0,0,-1],[-1,0,0]])
        wrist=world(local,wrist_name);wrist[:3,:3]=rotation;set_world(local,wrist_name,wrist)
        origin=wrist[:3,3]
        finger_roots=[world(local,'Bip01 '+side+' Finger'+str(i))[:3,3]-origin for i in (1,4)]
        palm_offset=(finger_roots[0]+finger_roots[1])*.35+unit(np.cross(*finger_roots))*(2 if side=='R' else -2)
        ik(local,'Bip01 '+side+' UpperArm','Bip01 '+side+' Forearm',wrist_name,goal-palm_offset,[(-40 if side=='L' else 42),12,55])
        wrist=world(local,wrist_name);wrist[:3,:3]=rotation;set_world(local,wrist_name,wrist)
        for finger in range(5):
            for joint in ('','1','2'):
                name=f'Bip01 {side} Thumb1{joint}' if finger==0 else f'Bip01 {side} Finger{finger}{joint}'
                if name in local:
                    angle=math.radians((45 if finger==0 else 65)*(-1 if side=='L' else 1))
                    local[name][:3,:3]=local[name][:3,:3]@rotate([0,0,1],angle)
    return local
def quat(matrix):
    # Stable conversion via the symmetric quaternion eigenproblem (xyzw).
    r=matrix;K=np.array([[r[0,0]-r[1,1]-r[2,2],r[0,1]+r[1,0],r[0,2]+r[2,0],r[2,1]-r[1,2]],
      [r[0,1]+r[1,0],r[1,1]-r[0,0]-r[2,2],r[1,2]+r[2,1],r[0,2]-r[2,0]],
      [r[0,2]+r[2,0],r[1,2]+r[2,1],r[2,2]-r[0,0]-r[1,1],r[1,0]-r[0,1]],
      [r[2,1]-r[1,2],r[0,2]-r[2,0],r[1,0]-r[0,1],np.trace(r)]])/3
    _,v=np.linalg.eigh(K);q=v[:,-1];return q if q[3]>=0 else -q
def author(distance,lateral=5):
    import stroke_study as stroke
    duration=stroke.DURATION
    times=np.unique(np.r_[np.linspace(0,duration,109),np.linspace(4.3,4.9,25),[k[0] for k in stroke.KEYS]])
    frames=[stroke.pose(t,distance,lateral) for t in times]
    data=N.Data(version=0x14020007,user_version=11,user_version_2=34)
    data.header.endian_type=1
    seq=N.NiControllerSequence();data.roots=[seq]
    seq.name=b'SpecialIdle_FCPool';seq.unknown_int_1=1;seq.weight=1;seq.cycle_type=2;seq.frequency=1;seq.start_time=0;seq.stop_time=duration;seq.target_name=b'Bip01'
    seq.text_keys=N.NiTextKeyExtraData();seq.text_keys.num_text_keys=2;seq.text_keys.text_keys.update_size()
    for key,t,value in zip(seq.text_keys.text_keys,[0,duration],[b'start',b'end']):key.time=t;key.value=value
    chosen=[name for name in nodes if name.startswith('Bip01 ')]
    seq.num_controlled_blocks=len(chosen);seq.controlled_blocks.update_size()
    for link,name in zip(seq.controlled_blocks,chosen):
        link.node_name=name.encode();link.controller_type=b'NiTransformController';link.priority=95 if 'Head' in name or 'Neck' in name else 50
        it=N.NiTransformInterpolator();link.interpolator=it;it.scale=1
        td=N.NiTransformData();it.data=td;td.num_rotation_keys=len(times);td.rotation_type=1;td.quaternion_keys.update_size()
        td.translations.num_keys=len(times);td.translations.interpolation=1;td.translations.keys.update_size()
        for j,(rot,trans) in enumerate(zip(td.quaternion_keys,td.translations.keys)):
            m=frames[j][name]
            # Runtime locomotion owns Bip01. Special-idle transforms relocate
            # the bind skeleton's root rotation/height into NonAccum (the same
            # coordinate contract as the owned native KF), without root motion.
            if name=='Bip01 NonAccum':m=frames[j]['Bip01']@m
            q=quat(m[:3,:3]);rot.time=times[j];rot.value.x,rot.value.y,rot.value.z,rot.value.w=map(float,q)
            trans.time=times[j];trans.value.x,trans.value.y,trans.value.z=map(float,m[:3,3])
    out=ROOT/'build/pool-vr/mod/meshes/Characters/_Male/IdleAnims/FilthyCasualPool'/f'SpecialIdle_FCPool_D{distance}_X{lateral}.kf';out.parent.mkdir(parents=True,exist_ok=True)
    with out.open('wb') as f:data.write(f)
    check=N.Data()
    with out.open('rb') as f:check.read(f)
    assert len(check.roots[0].controlled_blocks)==len(chosen)
    return {'distance':distance,'lateral':lateral,'path':str(out.relative_to(ROOT/'build/pool-vr/mod/meshes')).replace('/',chr(92))}
def author_item(item):
    distance,lateral,form=item
    result=author(distance,lateral);result['form']=form
    return result

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser();parser.add_argument('--workers',type=int,default=4);args=parser.parse_args()
    jobs=[(d,x,0x900+i) for i,(d,x) in enumerate((d,x) for d in range(50,181,5) for x in range(-30,31,5))]
    manifest=[]
    with ProcessPoolExecutor(max_workers=max(1,min(16,args.workers))) as workers:
        for result in workers.map(author_item,jobs):
            manifest.append(result)
            if len(manifest)%13==0:print('Authored pose distance',result['distance'],flush=True)
    (ROOT/'build/pool-vr/pose-bank.json').write_text(json.dumps(manifest,indent=2))
    print('Authored',len(manifest),'native stroke variants',flush=True)
