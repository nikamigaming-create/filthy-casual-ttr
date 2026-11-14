"""Inspect locally owned classic Vertibird assets. Never package generated data."""
import argparse, hashlib, json, sys, time, zlib, struct
from pathlib import Path
from PIL import Image
time.clock=time.perf_counter
from pyffi.formats.nif import NifFormat as N
sys.path.insert(0,r'D:\code\fnvvr\local\pydeps')
from bethesda_structs.archive.bsa import BSAArchive
p=argparse.ArgumentParser();p.add_argument('--game-data',type=Path,required=True);p.add_argument('--dependency-data',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
out=a.output;out.mkdir(parents=True,exist_ok=True);local=out/'Data';needed={'meshes/vehicles/vertibirdground01.nif','meshes/furniture/enclavechair01.nif','meshes/clutter/junk/motorhandle01.nif'}
def extract(needed):
 for archive in ('Fallout - Meshes.bsa','Fallout - Textures.bsa','Fallout - Textures2.bsa'):
  with (a.game_data/archive).open('rb') as f:
   c=BSAArchive.archive_struct.parse_stream(f);i=0
   for folder in c.directory_blocks:
    for r in folder.file_records:
     name=(folder.name.rstrip('\0').replace('\\','/')+'/'+c.file_names[i]).lower();i+=1
     if name not in needed:continue
     f.seek(r.offset);payload=f.read(r.size&0x3fffffff)
     if c.header.archive_flags.files_prefixed:payload=payload[1+payload[0]:]
     if bool(c.header.archive_flags.files_compressed)^bool(r.size&0x40000000):
      expected=struct.unpack('<I',payload[:4])[0];payload=zlib.decompress(payload[4:]);assert len(payload)==expected
     dest=local/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(payload)
extract(needed)
sources={'handle':local/'meshes/clutter/junk/motorhandle01.nif','chair':local/'meshes/furniture/enclavechair01.nif','exterior':local/'meshes/vehicles/vertibirdground01.nif','interior':a.dependency_data/'Meshes/PlayerVertibird/vertibirdinterior.nif','console':a.dependency_data/'Meshes/PlayerVertibird/vertibirdinterior_console.nif'}
meshes=[];provenance=[]
for group,path in sources.items():
 d=N.Data()
 with path.open('rb') as f:d.read(f)
 provenance.append({'group':group,'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
 for s in d.get_global_iterator():
  if not isinstance(s,N.NiGeometry) or not s.data or not hasattr(s.data,'vertices'):continue
  m=s.get_transform(d.roots[0]);vv=[v*m for v in s.data.vertices];texture=None;normal=None;alpha=1
  for prop in s.properties:
   if isinstance(prop,N.BSShaderPPLightingProperty) and prop.texture_set:
    ts=prop.texture_set.textures;texture=ts[0].decode().replace('\\','/').lower();normal=ts[1].decode().replace('\\','/').lower() if len(ts)>1 else None
   if isinstance(prop,N.NiMaterialProperty):alpha=prop.alpha
  needed.update(x for x in (texture,normal) if x)
  meshes.append({'group':group,'name':s.name.decode(),'vertices':[[v.x,v.y,v.z] for v in vv],'triangles':list(s.data.get_triangles()),'uvs':[[v.u,v.v] for v in s.data.uv_sets[0]] if s.data.num_uv_sets else [],'diffuse':texture,'normal':normal,'alpha':alpha})
extract(needed)
missing=[];textures={}
for rel in sorted(needed):
 if not rel.endswith('.dds'):continue
 path=a.dependency_data/rel
 if not path.is_file():path=local/rel
 if not path.is_file():missing.append(rel);continue
 png=out/'png'/Path(rel).with_suffix('.png');png.parent.mkdir(parents=True,exist_ok=True);Image.open(path).save(png)
 textures[rel]=str(png.resolve())
(out/'geometry.json').write_text(json.dumps({'meshes':meshes,'textures':textures,'sources':provenance,'missing':missing},separators=(',',':')),encoding='utf8')
print(json.dumps({'shapes':len(meshes),'triangles':sum(len(m['triangles']) for m in meshes),'missing':missing,'sources':provenance},indent=2))
if missing:raise SystemExit('Missing textures must be resolved before render')
