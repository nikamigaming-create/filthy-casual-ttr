"""Add a native-asset flight control to an already fitted private cockpit.
No source/derived game content is distributed. Fits the left pilot seat and stock-asset control without cutting the cabin.
"""
import argparse,time,io,copy
from pathlib import Path
time.clock=time.perf_counter
from pyffi.formats.nif import NifFormat as N
p=argparse.ArgumentParser();p.add_argument('--cockpit',type=Path,required=True);p.add_argument('--handle',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
def read(p):
 d=N.Data()
 with p.open('rb') as f:d.read(f)
 return d
def clone(obj):
 d=N.Data(version=0x14020007,user_version=11,user_version_2=34);d.header.endian_type=1;d.roots=[obj];stream=io.BytesIO();d.write(stream);stream.seek(0);other=N.Data();other.read(stream);return other.roots[0]
def node(name,pos):
 n=N.NiNode();n.name=name.encode();n.flags=14;n.scale=1;n.rotation.set_identity();n.translation.x,n.translation.y,n.translation.z=pos;return n
sk=read(a.cockpit);donor=read(a.handle);carrier=next(b for b in sk.get_global_iterator() if isinstance(b,N.NiNode) and b.name==b'FCCockpitCarrier')
for c in list(carrier.children):
 if c and c.name==b'FCFlightControl':carrier.remove_child(c)
# Pilot occupies the left seat, moved inward enough for the unchanged cabin
# rib and both forearms. Camera, native actor seat, chair and controls agree.
for name in [b'FCPilotSeatAnchor',b'FCPilotSeatRest',b'FCPilotEye']:
 n=next(b for b in sk.get_global_iterator() if isinstance(b,N.NiNode) and b.name==name);n.translation.x=-26
chair=next(b for b in sk.get_global_iterator() if isinstance(b,N.NiGeometry) and b.name==b'FCFalloutPilotChair-40');chair.translation.x=14
control=node('FCFlightControl',(-20,165,535));carrier.add_child(control)
# The stock motorcycle handgrip has its long axis along Y. Convert to upright
# control handles, with original triangles/UVs/materials and sensible hand scale.
src=next(b for b in donor.get_global_iterator() if isinstance(b,N.NiGeometry))
for hand,x in [('L',-7),('R',7)]:
 sh=clone(src);sh.name=('FCFlightHandle'+hand).encode();sh.skin_instance=None
 sh.rotation.set_identity();sh.translation.x,sh.translation.y,sh.translation.z=x,0,0;sh.scale=1
 for v in sh.data.vertices:
  xx,yy,zz=v.x,v.y,v.z;v.x=xx-.84;v.y=-(zz+14.445);v.z=yy-11.8
 for v in sh.data.normals:
  xx,yy,zz=v.x,v.y,v.z;v.x=xx;v.y=-zz;v.z=yy
 sh.data.update_center_radius();control.add_child(sh)
 control.add_child(node('FCFlightGrip'+hand,(x,0,0)))
# No duplicated console pedestal between the pilot knees. Keep only the
# handles; the original fitted cockpit already supplies the instrument panel.
a.output.parent.mkdir(parents=True,exist_ok=True)
with a.output.open('wb') as f:sk.write(f)
check=read(a.output);assert sum(isinstance(b,N.NiNode) and b.name==b'FCFlightControl' for b in check.get_global_iterator())==1
print(a.output)
