"""Export original Blender triangle data to FNV NIF20.2.0.7/user11/BS34.

Requires PyFFI2.2.3 and Pillow in Python3.11. Does not read any game assets.
Roundtrips both output variants and verifies topology, nodes, bounds and materials.
"""
import time
time.clock=time.perf_counter  # PyFFI2.2.3 compatibility with modern Python.
import json, math, random, hashlib
from collections import defaultdict
from pathlib import Path
from pyffi.formats.nif import NifFormat as N
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'wrist-controls'
SRC=json.loads((OUT/'geometry.json').read_text())
CONTRACT=json.loads((OUT/'attachment.json').read_text())
assert 'proxy' not in SRC['materials']
assert all('proxy' not in m['name'].lower() for m in SRC['meshes'])
SCALE=.6999125
MESH=OUT/'meshes'/'FilthyCasual';TEX=OUT/'textures'/'FilthyCasual'
MESH.mkdir(parents=True,exist_ok=True);TEX.mkdir(parents=True,exist_ok=True)
def srgb(x):return 12.92*x if x<=.0031308 else 1.055*x**(1/2.4)-.055
for name,color in SRC['materials'].items():
 rng=random.Random(name);im=Image.new('RGBA',(64,64));px=[]
 for y in range(64):
  for x in range(64):
   noise=rng.uniform(.91,1.07)
   if y%17==3 and x%19<7:noise*=1.15
   px.append(tuple(round(255*max(0,min(1,srgb(v)*noise))) for v in color)+(255,))
 im.putdata(px);im.save(TEX/(name+'.dds'))
Image.new('RGBA',(8,8),(128,128,255,80)).save(TEX/'flat_n.dds')

def node(name):
 ob=N.NiNode();ob.name=name.encode();ob.flags=14;ob.scale=1
 ob.rotation.set_identity();return ob
def color(dst,rgb):dst.r,dst.g,dst.b=rgb
def properties(mat):
 p=N.NiMaterialProperty();p.name=mat.encode();p.alpha=1;p.glossiness=22
 color(p.ambient_color,(1,1,1));color(p.diffuse_color,(1,1,1))
 color(p.specular_color,(.3,.3,.3));color(p.emissive_color,(0,0,0))
 s=N.BSShaderPPLightingProperty();s.shader_type=1;s.smooth=1
 s.shader_flags.sf_specular=1;s.shader_flags.sf_z_buffer_test=1
 s.shader_flags_2.sf_2_z_buffer_write=1
 t=N.BSShaderTextureSet();t.num_textures=6;t.textures.update_size()
 t.textures[0]=('textures\\FilthyCasual\\'+mat+'.dds').encode()
 t.textures[1]=b'textures\\FilthyCasual\\flat_n.dds';s.texture_set=t
 return p,s

groups=defaultdict(list)
for m in SRC['meshes']:groups[(m['group'],m['material'],bool(m.get('moving',False)))].append(m)
assert all(any(m['group']==b['node'] and m.get('moving') for m in SRC['meshes']) for b in CONTRACT['buttons'])
reports=[]
for mode in ('auto','manual'):
 d=N.Data(version=0x14020007,user_version=11,user_version_2=34)
 d.header.endian_type=1
 d.header.export_info.creator=b'Filthy Casual TTR'
 d.header.export_info.export_info_1=b'Original procedural geometry'
 d.header.export_info.export_info_2=b'Blender + PyFFI'
 root=node('FC_WristControls');d.roots=[root];parents={};press_parents={}
 for group in sorted(set(g for g,m,moving in groups)):
  p=node(group)
  if group=='FC_Reload_Auto' and mode=='manual' or group=='FC_Reload_Manual' and mode=='auto':p.flags|=1
  root.add_child(p);parents[group]=p
  if any(g==group and moving for g,m,moving in groups):
   press=node(group+'_Press');p.add_child(press);press_parents[group]=press
 for (group,mat,moving),parts in sorted(groups.items()):
  vertices=[];normals=[];triangles=[]
  for part in parts:
   start=len(vertices);vertices += [[v*SCALE for v in p] for p in part['vertices_cm']]
   normals+=part['normals'];triangles += [[v+start for v in t] for t in part['triangles']]
  # Preserve hard normals while sharing the two triangles on each flat face.
  unique={};verts2=[];norms2=[];remap=[]
  for p,n in zip(vertices,normals):
   key=tuple(round(v,6) for v in p+n)
   if key not in unique:unique[key]=len(verts2);verts2.append(p);norms2.append(n)
   remap.append(unique[key])
  vertices,normals=verts2,norms2;triangles=[[remap[i] for i in t] for t in triangles]
  shape=N.NiTriShape();shape.name=(group+('_Press_' if moving else '_Fixed_')+mat).encode();shape.flags=14;shape.scale=1;shape.rotation.set_identity()
  dat=N.NiTriShapeData();shape.data=dat
  dat.num_vertices=len(vertices);dat.has_vertices=True;dat.vertices.update_size()
  for dst,src in zip(dat.vertices,vertices):dst.x,dst.y,dst.z=src
  dat.has_normals=True;dat.normals.update_size()
  for dst,src in zip(dat.normals,normals):dst.x,dst.y,dst.z=src
  dat.num_uv_sets=1;dat.uv_sets.update_size()
  for dst,src in zip(dat.uv_sets[0],vertices):dst.u=src[0]*.8;dst.v=src[1]*.8
  dat.set_triangles(triangles);dat.update_center_radius()
  shape.update_tangent_space()
  # Fallout keeps per-geometry renderer state on shader properties. Never share
  # one shader/material object between shapes, even when DDS paths match.
  for prop in properties(mat):shape.add_property(prop)
  (press_parents[group] if moving else parents[group]).add_child(shape)
 path=MESH/('wrist-controls-'+mode+'.nif')
 with path.open('wb') as f:d.write(f)
 check=N.Data()
 with path.open('rb') as f:check.read(f)
 assert (check.version,check.user_version,check.user_version_2)==(0x14020007,11,34)
 nodes=[b for b in check.get_global_iterator() if isinstance(b,N.NiNode)]
 shapes=[b for b in check.get_global_iterator() if isinstance(b,N.NiTriShape)]
 assert len(shapes)==len(groups)
 shader_ids=[];material_ids=[]
 for sh in shapes:
  shaders=[p for p in sh.properties if isinstance(p,N.BSShaderPPLightingProperty)]
  materials=[p for p in sh.properties if isinstance(p,N.NiMaterialProperty)]
  assert len(shaders)==len(materials)==1
  shader_ids.append(id(shaders[0]));material_ids.append(id(materials[0]))
 assert len(set(shader_ids))==len(shapes)
 assert len(set(material_ids))==len(shapes)
 coords=[];nt=0
 for sh in shapes:
  dat=sh.data;assert dat.num_vertices and dat.num_triangles
  assert dat.num_vertices==len(dat.normals)==len(dat.uv_sets[0])
  assert dat.has_normals and dat.num_uv_sets==1
  for p in dat.vertices:
   assert all(math.isfinite(v) for v in (p.x,p.y,p.z));coords.append((p.x,p.y,p.z))
  for tri in dat.get_triangles():assert all(0<=i<dat.num_vertices for i in tri)
  for norm in dat.normals:assert abs(norm.x**2+norm.y**2+norm.z**2-1)<.002
  nt+=dat.num_triangles
 bounds=[[min(p[i] for p in coords),max(p[i] for p in coords)] for i in range(3)]
 assert abs((bounds[0][1]-bounds[0][0])/SCALE-7)<.02
 assert abs((bounds[1][1]-bounds[1][0])/SCALE-5)<.02
 for n in nodes:
  if n.name==b'FC_Reload_Auto':assert bool(n.flags&1)==(mode=='manual')
  if n.name==b'FC_Reload_Manual':assert bool(n.flags&1)==(mode=='auto')
 node_by_name={n.name.decode():n for n in nodes}
 for b in CONTRACT['buttons']:
  parent=node_by_name[b['node']];press=node_by_name[b['press_node']]
  assert press in parent.children
  assert (press.translation.x,press.translation.y,press.translation.z)==(0,0,0)
  assert press.scale==1 and press.rotation.is_identity()
  moving_shapes=[c for c in press.children if isinstance(c,N.NiTriShape)]
  fixed_shapes=[c for c in parent.children if isinstance(c,N.NiTriShape)]
  assert moving_shapes and fixed_shapes
  assert all('_Press_' in s.name.decode() for s in moving_shapes)
  assert all('_Fixed_' in s.name.decode() for s in fixed_shapes)
  # Contact face is the broad plunger enamel, not the fixed raised collar.
  source_cap=next(m for m in SRC['meshes'] if m['group']==b['node'] and 'enamel plunger' in m['name'])
  assert abs(max(p[2] for p in source_cap['vertices_cm'])-b['contact_surface_z_cm'])<1e-5
  fixed_before=[tuple((v.x,v.y,v.z)) for s in fixed_shapes for v in s.data.vertices]
  press.translation.z=-CONTRACT['travel_cm']*SCALE
  assert abs(press.translation.z+.111986)<1e-6
  assert fixed_before==[tuple((v.x,v.y,v.z)) for s in fixed_shapes for v in s.data.vertices]
  press.translation.z=0
 texture_paths=set()
 for block in check.get_global_iterator():
  if isinstance(block,N.BSShaderTextureSet):
   for texture in block.textures:
    if texture:
     relative=texture.decode().replace('\\','/')
     assert relative.startswith('textures/FilthyCasual/')
     assert (OUT/relative).is_file();texture_paths.add(relative)
 reports.append({'file':str(path.relative_to(OUT)),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
  'nodes':[n.name.decode() for n in nodes],'shapes':len(shapes),'triangles_including_hidden_mode':nt,
  'bytes':path.stat().st_size,'bounds_game_units':bounds,'roundtrip_pass':True,
  'press_routing_pass':True,'press_travel_cm':CONTRACT['travel_cm'],'contact_planes_pass':True,
  'unique_shader_per_shape':True,'unique_material_per_shape':True,
  'textures':sorted(texture_paths),'no_proxy':True,'runtime_verified':False})
(OUT/'validation.json').write_text(json.dumps({'format':'20.2.0.7 user11 BS34','original_asset':True,
 'design':'approved curved cuff','source_script':'tools/build_wrist_cuff.py',
 'geometry_sha256':hashlib.sha256((OUT/'geometry.json').read_bytes()).hexdigest(),
 'attachment_sha256':hashlib.sha256((OUT/'attachment.json').read_bytes()).hexdigest(),'results':reports},indent=2))
print(json.dumps(reports,indent=2))
