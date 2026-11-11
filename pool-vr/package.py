"""Build a standalone distribution; never package locally generated upstream scripts."""
from pathlib import Path
import hashlib,json,zipfile
R=Path(__file__).resolve().parents[1]; P=R/'pool-vr'; B=R/'build/pool-vr'; M=B/'mod'
files={}
for p in (M/'meshes/Characters/_Male/IdleAnims/FilthyCasualPool').glob('SpecialIdle_FCPool_*.kf'):
 files[p.relative_to(M).as_posix()]=p
assert len(files)==351, 'Expected the complete newly authored animation bank'
files['Filthy Casual Pool.esp']=M/'Filthy Casual Pool.esp'
for name in ['Activate','Close','Init','OpponentAvailable','ReleaseSunny','ResetProps','Running','SunnyTick','SunnyTopic']:
 rel='NVSE/user_defined_functions/'+('Minigames/Pool/' if name=='Activate' else 'FilthyCasualPool/')+name+'.txt'
 files[rel]=M/rel
files['NVSE/Plugins/FilthyCasualPool.dll']=B/'FilthyCasualPool.dll'
files['NVSE/Plugins/FilthyCasualPool/FilthyCasualPoolInput.dll']=B/'FilthyCasualPoolInput.dll'
files['NVSE/Plugins/FilthyCasualPool/FilthyCasualPoolInput.json']=P/'FilthyCasualPoolInput.json'
for name in ['README.md','Prepare-Pool.ps1','Prepare-Pool.cmd','meta.ini']:files[name]=P/name
for name in ['Start.template','Tick.template','CandidateInsertion.txt','recipe.json']:files['setup/'+name]=P/'setup'/name
files['LICENSE']=R/'LICENSE'
for name in ['Apache-2.0.txt','MIT.txt']:files['licenses/OpenXR-'+name]=R/'third_party/openxr/LICENSES'/name
assert all(p.is_file() for p in files.values())
manifest={'addon':'Filthy Casual Pool','version':'0.2.0-beta','standalone':True,
 'supportedTTWVR':'E428C4BD1209D58D21F75108808C86049E26C76E58A164AD864A61B99696BDAB',
 'requires':['Working supported TTR setup','Pool Tables Of The Mojave v4'],
 'optional':['Filthy Casual TTR >=1.4.5'],
 'generatedLocally':['Start.txt','Tick.txt','PlanCandidate.txt'],
 'files':[{'path':n,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for n,p in sorted(files.items())]}
output=R/'outputs/Filthy-Casual-Pool-0.2.0-beta.zip';output.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for n in sorted([*files,'manifest.json']):
  b=json.dumps(manifest,indent=2).encode() if n=='manifest.json' else files[n].read_bytes()
  i=zipfile.ZipInfo(n,(2000,1,1,0,0,0));i.compress_type=zipfile.ZIP_DEFLATED;z.writestr(i,b)
print(output)
