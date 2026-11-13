"""Attach our original cockpit to the user's locally installed aircraft rig.
The resulting skeleton incorporates dependency data and is never distributed.
"""
from pathlib import Path
import time,hashlib,json,shutil,argparse
parser=argparse.ArgumentParser();parser.add_argument("--owned-fo4",action="store_true");args=parser.parse_args()
time.clock=time.perf_counter
from pyffi.formats.nif import NifFormat as N
ROOT=Path(__file__).resolve().parents[1];asset=ROOT/'assets/vertibird-cockpit'
source=Path(r'D:\TTWVR\MO2\mods\Vertibird Player Home 4.18\Meshes\PlayerVertibird\Creature\Vertibird\skeleton.nif')
mod=Path(r'D:\TTWVR\MO2\mods\Filthy Casual Vehicles - Development')
out=mod/'Meshes/PlayerVertibird/Creature/Vertibird/skeleton.nif'
d=N.Data()
with source.open('rb') as f:d.read(f)
root=d.roots[0];spine=next(b for b in d.get_global_iterator() if isinstance(b,N.NiNode) and b.name==b'Bip01 Spine1')
old=next(b for b in d.get_global_iterator() if isinstance(b,N.NiNode) and b.name==b'PlayerPos');old.name=b'FCOriginalPilotPos'
carrier=N.NiNode();carrier.name=b'FCCockpitCarrier';carrier.flags=14;carrier.set_transform(spine.get_transform(root).get_inverse());spine.add_child(carrier)
a=N.Data()
mesh=ROOT/'work/vertibird/fo4/converted/vertibird.nif' if args.owned_fo4 else asset/'meshes/FilthyCasual/vertibird-cockpit.nif'
with mesh.open('rb') as f:a.read(f)
carrier.add_child(a.roots[0])
contract=json.loads((ROOT/'work/vertibird/fo4/conversion.json' if args.owned_fo4 else asset/'contract.json').read_text())
anchor=N.NiNode();anchor.name=b'FCPilotSeatAnchor';anchor.flags=14;anchor.scale=1;anchor.rotation.set_identity();anchor.translation.x,anchor.translation.y,anchor.translation.z=contract['pilot_anchor']
seat=N.NiNode();seat.name=b'PlayerPos';seat.flags=14;seat.scale=1;seat.rotation.set_identity();seat.translation.y=-128
anchor.add_child(seat);carrier.add_child(anchor)
rest=N.NiNode();rest.name=b'FCPilotSeatRest';rest.flags=14;rest.set_transform(anchor.get_transform());carrier.add_child(rest)
out.parent.mkdir(parents=True,exist_ok=True)
with out.open('wb') as f:d.write(f)
shutil.copytree(ROOT/'work/vertibird/fo4/converted/textures' if args.owned_fo4 else asset/'textures',mod/'Textures',dirs_exist_ok=True)
(ROOT/'work/vertibird/cockpit-import.json').write_text(json.dumps({'dependency':str(source),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'cockpit_sha256':hashlib.sha256(mesh.read_bytes()).hexdigest(),'output':str(out),'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'note':'Local dependency-derived skeleton only; never distribute.'},indent=2))
print(out)
