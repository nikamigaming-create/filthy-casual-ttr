from pathlib import Path
import time,hashlib,json
time.clock=time.perf_counter
from pyffi.formats.nif import NifFormat as N
source=Path(r'D:\TTWVR\MO2\mods\Uhmatt Drivable Cars 8b\Meshes\Creatures\MkTRoadster\Roadster.nif')
out=Path(r'D:\TTWVR\MO2\mods\Filthy Casual Vehicles - Development\Meshes\Creatures\MkTRoadster\Roadster.nif')
d=N.Data()
with source.open('rb') as f:d.read(f)
root=d.roots[0];wheel=next(b for b in d.get_global_iterator() if isinstance(b,N.NiGeometry) and b.name==b'SteeringWheelA')
center=(-19.653015,-22.371404,82.346992)
pivot=N.NiNode();pivot.name=b'FCSteeringWheel';pivot.flags=14;pivot.scale=1;pivot.rotation.set_identity();pivot.translation.x,pivot.translation.y,pivot.translation.z=center
for node in d.get_global_iterator():
 if isinstance(node,N.NiNode) and wheel in list(node.children):node.remove_child(wheel);break
assert len(wheel.skin_instance.bones)==1
bone_name=wheel.skin_instance.bones[0].name
inverse_bind=wheel.skin_instance.data.bone_list[0].get_transform()
assert abs(wheel.scale-1)<1e-6 and wheel.rotation.is_identity()
wheel.skin_instance=None
# The geometry is now rigid; a skinned shader would read a nonexistent palette.
for prop in wheel.properties:
 if hasattr(prop,'shader_flags'):prop.shader_flags.sf_skinned=0
for v in wheel.data.vertices:v.x-=center[0];v.y-=center[1];v.z-=center[2]
wheel.data.update_center_radius();pivot.add_child(wheel)
# Actor mesh attachment keeps skinned geometry, so place the rigid wheel in the
# dependency skeleton. This is still a local import, never a packaged asset.
skel=N.Data()
with (source.parent/'Skeleton.nif').open('rb') as f:skel.read(f)
# Preserve the source skin's authored inverse bind. The actor skeleton uses a
# different bind stance, so attaching this directly to Scene Root is incorrect.
bone=next(b for b in skel.get_global_iterator() if isinstance(b,N.NiNode) and b.name==bone_name)
carrier=N.NiNode();carrier.name=b'FCWheelCarrier';carrier.flags=14;carrier.set_transform(inverse_bind)
bone.add_child(carrier);carrier.add_child(pivot)
# The authored rim/column is a second shape. Keep both pieces together.
second=next(b for b in d.get_global_iterator() if isinstance(b,N.NiGeometry) and b.name==b'SteeringWheelB')
for parent in d.get_global_iterator():
 if isinstance(parent,N.NiNode) and second in list(parent.children):parent.remove_child(second);break
second.skin_instance=None
for prop in second.properties:
 if hasattr(prop,'shader_flags'):prop.shader_flags.sf_skinned=0
for v in second.data.vertices:v.x-=center[0];v.y-=center[1];v.z-=center[2]
second.data.update_center_radius();pivot.add_child(second)


# Keep the source driver's authored X/Y location. Correct full-frame transforms
# make its original 28-unit forward wheel reach usable without lateral shifts.
seat=next(b for b in skel.get_global_iterator() if isinstance(b,N.NiNode) and b.name==b'Seat')
# Actor root remains on the authored floor. Eye fit is a separate tracked anchor.
eye=N.NiNode();eye.name=b'FCDriverEye';eye.flags=14;eye.scale=1;eye.rotation.set_identity()
eye.translation.x=-20;eye.translation.y=-50;eye.translation.z=97
carrier.add_child(eye)

# The wheel and chassis are animated by the source spine bone. The original
# Seat lives outside that branch and drifts during Forward/FastForward clips.
# Preserve its bind position, but carry it with the same body transform.
old_parent=next(n for n in skel.get_global_iterator() if isinstance(n,N.NiNode) and seat in list(n.children))
seat_bind=seat.get_transform(skel.roots[0])
carrier_bind=carrier.get_transform(skel.roots[0])
seat.set_transform(seat_bind*carrier_bind.get_inverse())
old_parent.remove_child(seat);carrier.add_child(seat)
# Original bench surface at driver X/Y is Z57.53. Preserve the same 6.99-unit
# pelvis-to-seat support offset measured on the native Vertibird chair; the
# native seated clip pelvis is36.85 above its root. Camera anchor stays separate.
seat.translation.z=57.53+6.99-36.85


out.parent.mkdir(parents=True,exist_ok=True)
with out.open('wb') as f:d.write(f)
with out.with_name('Skeleton.nif').open('wb') as f:skel.write(f)
Path('work/vehicles/local-wheel-import.json').write_text(json.dumps({'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'output_sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'note':'Locally derived dependency mesh. Never redistribute.'},indent=2))
print(out)
