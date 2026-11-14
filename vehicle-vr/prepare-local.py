"""Build a PRIVATE MO2 override from owned game/dependency files. No downloads."""
from pathlib import Path
import argparse,os,sys,subprocess,shutil,hashlib,json
p=argparse.ArgumentParser()
p.add_argument('--game-data',type=Path,required=True)
p.add_argument('--cars',type=Path)
p.add_argument('--vertibird',type=Path)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args();root=Path(__file__).resolve().parent
out=a.output.resolve()
if out.exists() and any(out.iterdir()):p.error('Choose a new empty output folder; existing mods are never overwritten.')
if not a.cars and not a.vertibird:p.error('Specify --cars and/or --vertibird extracted dependency folder.')
if not (a.game_data/'Fallout - Meshes.bsa').is_file():p.error('game-data must be the New Vegas Data folder.')
out.mkdir(parents=True,exist_ok=True);work=out/'.local-import';work.mkdir()
env=os.environ.copy();env['FC_OUTPUT_DATA']=str(out);env['FC_IMPORT_REPORT']=str(work/'car.json')
def run(script,*args):subprocess.run([sys.executable,str(root/script),*map(str,args)],env=env,check=True)
if a.cars:
 env['FC_CAR_DATA']=str(a.cars.resolve());run('import-roadster-local.py')
if a.vertibird:
 env['FC_FLIGHT_DATA']=str(a.vertibird.resolve());env['FC_IMPORT_WORK']=str(work)
 run('classic-vertibird/prepare_local.py','--game-data',a.game_data,'--dependency-data',a.vertibird,'--output',work)
 shutil.copy2(root/'classic-vertibird/body-alignment.json',work/'body-alignment.json')
 run('classic-vertibird/fit_cockpit.py')
 target=out/'Meshes/PlayerVertibird/Creature/Vertibird/skeleton.nif'
 run('add-flight-control-local.py','--cockpit',work/'runtime/skeleton.nif','--handle',work/'Data/meshes/clutter/junk/motorhandle01.nif','--output',target)
 shutil.copy2(work/'runtime/PlayerVertibird.nif',out/'Meshes/PlayerVertibird/PlayerVertibird.nif')
 shutil.copytree(work/'Data/textures',out/'textures',dirs_exist_ok=True)
files=[{'path':str(f.relative_to(out)).replace('\\','/'),'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for folder in ['Meshes','textures'] if (out/folder).exists() for f in (out/folder).rglob('*') if f.is_file()]
(out/'LOCAL-ONLY.json').write_text(json.dumps({'redistributable':False,'files':files},indent=2))
print('Ready: enable this private override below Filthy Casual Vehicles in MO2:',out)
print('Do not share this folder: it contains your locally derived game/dependency assets.')
