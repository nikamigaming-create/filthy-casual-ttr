"""Fit a classic transparent canopy and cockpit to the original animated rig.
All derived NIFs stay local; source game/dependency assets are never redistributable.
"""
import time,json,hashlib,subprocess,sys,copy,math,shutil
from pathlib import Path
import numpy as np
time.clock=time.perf_counter
from pyffi.formats.nif import NifFormat as N
ROOT=Path(__file__).resolve().parents[2];W=ROOT/'work/vertibird/classic';OUT=W/'runtime';OUT.mkdir(exist_ok=True)
DEP=Path(r'D:\TTWVR\MO2\mods\Vertibird Player Home 4.18');MOD=Path(r'D:\TTWVR\MO2\mods\Filthy Casual Vehicles - Development')
def read(p):
 d=N.Data()
 with p.open('rb') as f:d.read(f)
 return d
def clone(p):
 import io
 tmp=N.Data(version=0x14020007,user_version=11,user_version_2=34);tmp.header.endian_type=1
 holder=N.NiNode();holder.name=b'Property copy';holder.scale=1;holder.rotation.set_identity();holder.add_property(p);tmp.roots=[holder]
 stream=io.BytesIO();tmp.write(stream);stream.seek(0);fresh=N.Data();fresh.read(stream);return fresh.roots[0].properties[0]
def node(name):
 n=N.NiNode();n.name=name.encode();n.flags=14;n.scale=1;n.rotation.set_identity();return n
source=DEP/'Meshes/PlayerVertibird/Creature/Vertibird/Vertibird.nif';d=read(source);body=next(b for b in d.get_global_iterator() if isinstance(b,N.NiGeometry) and b.name==b'BodyMain')
assert len(body.skin_instance.bones)==1
alignment=json.loads((W/'body-alignment.json').read_text());assert alignment['median']<.5
R=np.array(alignment['R']);T=np.array(alignment['t']);verts=(np.array([[v.x,v.y,v.z] for v in body.data.vertices])-T)@R
props=list(body.properties);shader=next(p for p in props if isinstance(p,N.BSShaderPPLightingProperty));textures=[v.decode().replace('\\','/').lower() for v in shader.texture_set.textures]
mesh={'name':'FCClassicHull','group':'exterior','vertices':verts.tolist(),'triangles':list(body.data.get_triangles()),'uvs':[[v.u,v.v] for v in body.data.uv_sets[0]],'diffuse':textures[0],'normal':textures[1],'alpha':1}
tail=next(b for b in d.get_global_iterator() if isinstance(b,N.NiGeometry) and b.name==b'BodyTail')
assert tail.skin_instance.bones[0].name==body.skin_instance.bones[0].name
tail_shader=next(p for p in tail.properties if isinstance(p,N.BSShaderPPLightingProperty))
tail_mesh={**mesh,'name':'FCClassicHullTail','vertices':((np.array([[v.x,v.y,v.z] for v in tail.data.vertices])-T)@R).tolist(),'triangles':list(tail.data.get_triangles()),'uvs':[[v.u,v.v] for v in tail.data.uv_sets[0]],'diffuse':tail_shader.texture_set.textures[0].decode(),'normal':tail_shader.texture_set.textures[1].decode()}
(OUT/'geometry.json').write_text(json.dumps({'meshes':[mesh,tail_mesh],'textures':{},'sources':[{'path':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()}]}))
subprocess.run([sys.executable,str(Path(__file__).with_name('canopy_variant.py')),str(OUT)],check=True)
shapes=json.loads((OUT/'canopy-geometry.json').read_text())['meshes']
sk=read(DEP/'Meshes/PlayerVertibird/Creature/Vertibird/skeleton.nif');root=sk.roots[0];spine=next(b for b in sk.get_global_iterator() if isinstance(b,N.NiNode) and b.name==body.skin_instance.bones[0].name)
old=next(b for b in sk.get_global_iterator() if isinstance(b,N.NiNode) and b.name==b'PlayerPos');old.name=b'FCOriginalPilotPos'
carrier=node('FCCockpitCarrier');carrier.set_transform(body.skin_instance.data.bone_list[0].get_transform());spine.add_child(carrier)
carrier.add_child(node('FCClassicCockpit'))
report=[]
def shape(name,vertices,triangles,uvs,properties,normals=None):
 # Weld identical vertex/UV values so copied panel splits retain smooth normals.
 unique={};vv=[];tt=[];uu=[];nn=[];remap=[]
 used={i for t in triangles for i in t}
 for i,v in enumerate(vertices):
  if i not in used:remap.append(-1);continue
  uv=uvs[i] if uvs else [v[0]*.02,v[1]*.02];key=tuple(round(x,5) for x in list(v)+list(uv)+(list(normals[i]) if normals is not None else []))
  if key not in unique:unique[key]=len(vv);vv.append(v);uu.append(uv);nn.append(normals[i] if normals is not None else [0,0,1])
  remap.append(unique[key])
 for t in triangles:
  ids=[remap[i] for i in t]
  if len(set(ids))==3:tt.append(ids)
 if not tt:return
 assert len(vv)<65536 and len(tt)<65536
 sh=N.NiTriShape();sh.name=name.encode();sh.flags=14;sh.scale=1;sh.rotation.set_identity();dat=N.NiTriShapeData();sh.data=dat;dat.num_vertices=len(vv);dat.has_vertices=True;dat.vertices.update_size()
 for dst,v in zip(dat.vertices,vv):dst.x,dst.y,dst.z=v
 dat.num_uv_sets=1;dat.uv_sets.update_size()
 for dst,v in zip(dat.uv_sets[0],uu):dst.u,dst.v=v
 dat.set_triangles(tt);dat.has_normals=True;dat.normals.update_size()
 if normals is None:
  positions=np.array(vv);norm=np.zeros_like(positions)
  for t in tt:
   n=np.cross(positions[t[1]]-positions[t[0]],positions[t[2]]-positions[t[0]])
   for i in t:norm[i]+=n
  lengths=np.linalg.norm(norm,axis=1);norm[lengths>1e-10]/=lengths[lengths>1e-10,None];norm[lengths<=1e-10]=[0,0,1]
  for dst,n in zip(dat.normals,norm):dst.x,dst.y,dst.z=n
 else:
  for dst,n in zip(dat.normals,nn):dst.x,dst.y,dst.z=n
 dat.update_center_radius();sh.update_tangent_space()
 for p in properties:sh.add_property(p)
 carrier.add_child(sh);report.append({'name':name,'vertices':len(vv),'triangles':len(tt)})
 return sh
for m in shapes:
 if not m['triangles']:continue
 part_props=list(tail.properties) if m['name'].startswith('FCClassicHullTail') else props
 ps=[clone(p) for p in part_props if not isinstance(p,N.NiAlphaProperty)]
 for p in ps:
  if hasattr(p,'shader_flags'):p.shader_flags.sf_skinned=0
 if m.get('glass'):
  for p in ps:
   if isinstance(p,N.NiMaterialProperty):p.alpha=.045;p.glossiness=45;p.emissive_color.r=p.emissive_color.g=p.emissive_color.b=0
   if isinstance(p,N.BSShaderPPLightingProperty):
    p.texture_set.textures[0]=b'textures\\dungeons\\vault\\vglass01.dds';p.texture_set.textures[1]=b'textures\\dungeons\\vault\\vglass01_n.dds';p.shader_flags_2.sf_2_z_buffer_write=0;p.shader_flags.sf_glow_map=0
    for slot in range(2,len(p.texture_set.textures)):p.texture_set.textures[slot]=b''
  a=N.NiAlphaProperty();a.flags=4845;a.threshold=0;ps.append(a)
 st=N.NiStencilProperty();st.flags=19840 if m.get('glass') else 17792;st.draw_mode=3 if m.get('glass') else 1;ps.append(st)
 vv=(np.array(m['vertices'])@R.T+T).tolist();shape(m['name'],vv,m['triangles'],m['uvs'],ps)
# Use Fallout/VPH meshes and their original material bindings for the interior.
# Only the window aperture and attachment transforms are custom.
source_geometry=json.loads((W/'geometry.json').read_text())
by_group={}
for group,path in [('chair',W/'Data/meshes/furniture/enclavechair01.nif'),('console',DEP/'Meshes/PlayerVertibird/vertibirdinterior_console.nif'),('interior',DEP/'Meshes/PlayerVertibird/vertibirdinterior.nif')]:
 source_data=read(path);by_group[group]={b.name.decode():b for b in source_data.get_global_iterator() if isinstance(b,N.NiGeometry)}
# Keep source lining, fitted against the uncut hull before cutting shared windows.
from cabin_fit import CabinFit,cabin_depth
uncut=np.concatenate([(np.array(m['vertices'])@R.T+T)[np.array(m['triangles'])] for m in (mesh,tail_mesh)])
source_lining=np.concatenate([(np.array(m['vertices'])*[-.75,-.75,.75]+[0,90,582])[np.array(m['triangles'])] for m in source_geometry['meshes'] if m['name'] in ('VertibirdHull01:0','VertibirdHull01:1')])
cabin_fit=CabinFit(uncut,source_lining,R,T)

for m in source_geometry['meshes']:
 group=m['group']
 if group not in by_group or m['name']=='VertibirdHull01:2':continue
 original=by_group[group][m['name']];ps=[clone(p) for p in original.properties]
 for p in ps:
  if hasattr(p,'shader_flags'):p.shader_flags.sf_skinned=0
 vertices=np.array(m['vertices']);triangles=m['triangles']
 if group=='chair':
  for x in (-40,40):
   shape('FCFalloutPilotChair'+str(x),(vertices+np.array([x,130,527])).tolist(),triangles,m['uvs'],ps)
 elif group=='console':
  vv=vertices*np.array([-.75,-.75,.75])+np.array([0,90,582])
  upper=[t for t in triangles if all(vertices[i,2]>-45 for i in t)]
  lower=[t for t in triangles if not all(vertices[i,2]>-45 for i in t)]
  assert not any(min(vertices[i,2] for i in t)<-45<max(vertices[i,2] for i in t) for t in triangles)
  cv,ct,cu=cabin_fit.fit(m['name'],vv,lower,m['uvs'],cut_windows=False)
  shape('FCFalloutConsole_'+m['name'],cv.tolist(),ct,cu,ps)
  if upper:
   assert m['name']=='VertibirdConsole01:0' and len(upper)==184
   shape('FCFalloutOverheadControls',(vv+np.array([0,-55,55])).tolist(),upper,m['uvs'],[clone(p) for p in ps])
 else:
  if m['name']=='VertibirdHull01:0':
   # Restore the original folded overhead-console housing, removed from the old
   # whole-cabin clipping. Selection is bounded and rejects changed source data.
   selected=[t for t in triangles if all(abs(vertices[i,0])<=21.4 and -165.04<=vertices[i,1]<=-121.96 and -38.31<=vertices[i,2]<=-18.59 for i in t)]
   assert len(selected)==20, ('Unexpected overhead housing',len(selected))
   # The source cockpit's transverse window return is replaced by the fitted
   # canopy ribs. Keeping its central remnant produced an unsupported plate.
   old_return_ids=(693,694,695,949,950,951)
   old_returns=[triangles[i] for i in old_return_ids]
   assert all(-113.89<=vertices[i,1]<=-112.79 and -16.64<=vertices[i,2]<=-.52 and abs(vertices[i,0])<50 for t in old_returns for i in t), 'Unexpected source window return'
   old_apron=[m['triangles'][i] for i in (903,904,1286,1287)]
   assert all(abs(vertices[i,0])<=21.39 and -193.95<=vertices[i,1]<=-154.63 and -48.77<=vertices[i,2]<=-38.30 for t in old_apron for i in t), 'Unexpected source console apron'
   # These four faces linked the old ceiling panel to the forward terminal.
   # The relocated complete casing has its own closed shell and roof mounts;
   # the terminal keeps its original enclosure. Do not leave a severed bridge.
   replaced={tuple(t) for t in old_returns+old_apron};triangles=[t for t in triangles if tuple(t) not in replaced]

   moved={tuple(t) for t in selected};triangles=[t for t in triangles if tuple(t) not in moved]
   # All20 source casing faces support the complete184-face control assembly.
   # The forward apron backs a second control row at runtimeY150.68; trimming
   # it exposed that row as two bright floating plates. Preserve it intact.
   panel_vertices=[];panel_uv=[];pt=[]
   for tri in selected:
    start=len(panel_vertices);panel_vertices.extend(vertices[i] for i in tri);panel_uv.extend(m['uvs'][i] for i in tri);pt.append([start,start+1,start+2])
   pv=np.array(panel_vertices)*np.array([-.75,-.75,.75])+np.array([0,35,637]);pu=np.array(panel_uv)
   # Give the source's open-back housing a closed two-unit shell. Weld only edge
   # matching, keeping the original UV seams on its weathered metal panels.
   from collections import defaultdict
   edges=defaultdict(list)
   for t in pt:
    for a,b in zip(t,t[1:]+t[:1]):
     key=tuple(sorted((tuple(np.round(pv[a],4)),tuple(np.round(pv[b],4)))))
     edges[key].append((a,b))
   n=len(pv);closed=pt+[[i+n for i in reversed(t)] for t in pt]
   for e in edges.values():
    if len(e)==1:
     a,b=e[0];closed.extend([[a,b,b+n],[a,b+n,a+n]])
   shape('FCFalloutOverheadHousing',np.concatenate([pv,pv+[0,0,2]]).tolist(),closed,np.concatenate([pu,pu]).tolist(),[clone(p) for p in ps])
   # Two short mount lugs join the complete housing to the real retained roof rib.
   # Use the original housing material and UV patch, not an untextured overlay.
   metal_uv=pu[np.argmin(np.linalg.norm(pv-np.array([0,127,623]),axis=1))]
   for x in (-10,10):
    box=[[x+dx,127+dy,z] for z in (623,632) for dy in (-1.2,1.2) for dx in (-1.2,1.2)]
    faces=[[0,2,3],[0,3,1],[4,5,7],[4,7,6],[0,1,5],[0,5,4],[2,6,7],[2,7,3],[0,4,6],[0,6,2],[1,3,7],[1,7,5]]
    uv=[(metal_uv+np.array([dx,dy])*.01).tolist() for z in range(2) for dy in (0,1) for dx in (0,1)]
    shape('FCFalloutOverheadMount'+str(x),box,faces,uv,[clone(p) for p in ps])
  vv=vertices*np.array([-.75,-.75,.75])+np.array([0,90,582])
  vv,triangles,cabin_uvs=cabin_fit.fit(m['name'],vv,triangles,m['uvs'])
  if triangles:shape('FCFalloutCabin_'+m['name'],vv.tolist(),triangles,cabin_uvs,ps)
(OUT/'cabin-fit-diagnostics.json').write_text(json.dumps(cabin_fit.stats,indent=2));print(cabin_fit.stats)
assert all(s['collapsed']==0 and s['inverted']==0 for s in cabin_fit.stats), 'Reject invalid fitted cabin surface'

eye=[40.,140,566.];anchor=node('FCPilotSeatAnchor');anchor.translation.x=eye[0];anchor.translation.y=eye[1]+128;anchor.translation.z=eye[2]-77
seat=node('PlayerPos');seat.translation.y=-128;anchor.add_child(seat);carrier.add_child(anchor);rest=node('FCPilotSeatRest');rest.set_transform(anchor.get_transform());carrier.add_child(rest)
eye_node=node('FCPilotEye');eye_node.translation.x,eye_node.translation.y,eye_node.translation.z=eye;carrier.add_child(eye_node)
output=OUT/'skeleton.nif'
with output.open('wb') as f:sk.write(f)
check=read(output);allsh=[b for b in check.get_global_iterator() if isinstance(b,N.NiGeometry)];assert len(allsh)==len(report)
for sh in allsh:
 assert all(math.isfinite(c) for v in sh.data.vertices for c in (v.x,v.y,v.z))
 assert not sh.skin_instance
 for p in sh.properties:
  if hasattr(p,'shader_flags'):assert not p.shader_flags.sf_skinned
record={'stage':'local NIF candidate','source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'output_sha256':hashlib.sha256(output.read_bytes()).hexdigest(),'pilot_eye':eye,'pilot_anchor':[eye[0],eye[1]+128,eye[2]-77],'retained_native_rotors_and_gear':True,'native_body_inverse_bind':True,'shapes':report,'roundtrip':True,'private_only':True}
(OUT/'fit-contract.json').write_text(json.dumps(record,indent=2));print(json.dumps({'shapes':len(report),'triangles':sum(v['triangles'] for v in report),'eye':eye,'output':str(output)}))
# Match parked/landed canopy while retaining every native controller and rotor.
fitparts=[b for b in carrier.children if isinstance(b,N.NiGeometry) and not b.name.startswith(b'FCClassicHull')]
pd=read(DEP/'Meshes/PlayerVertibird/PlayerVertibird.nif');parkroot=pd.roots[0]
originals=[b for b in pd.get_global_iterator() if isinstance(b,N.NiGeometry) and b.name in (b'ModelSwapNode:0',b'ModelSwapNode:1')]
parkdir=OUT/'parked';parkdir.mkdir(exist_ok=True);parkmeshes=[]
for b in originals:
 tex=next(p for p in b.properties if isinstance(p,N.BSShaderPPLightingProperty)).texture_set.textures
 parkmeshes.append({'name':b.name.decode(),'group':'exterior','vertices':[[v.x,v.y,v.z+225.04] for v in b.data.vertices],'triangles':list(b.data.get_triangles()),'uvs':[[v.u,v.v] for v in b.data.uv_sets[0]],'diffuse':tex[0].decode(),'normal':tex[1].decode(),'alpha':1})
(parkdir/'geometry.json').write_text(json.dumps({'meshes':parkmeshes}))
subprocess.run([sys.executable,str(Path(__file__).with_name('canopy_variant.py')),str(parkdir)],check=True)
parkshapes=json.loads((parkdir/'canopy-geometry.json').read_text())['meshes']
for orig in originals:
 parent=next(b for b in pd.get_global_iterator() if isinstance(b,N.NiNode) and orig in list(b.children));carrier=parent
 for m in parkshapes:
  if not m['name'].startswith(orig.name.decode()) or not m['triangles']:continue
  ps=[clone(p) for p in orig.properties if not isinstance(p,(N.NiAlphaProperty,N.NiStencilProperty))]
  if m.get('glass'):
   for p in ps:
    if isinstance(p,N.NiMaterialProperty):p.alpha=.045;p.glossiness=45;p.emissive_color.r=p.emissive_color.g=p.emissive_color.b=0
    if isinstance(p,N.BSShaderPPLightingProperty):
     p.texture_set.textures[0]=b'textures\\dungeons\\vault\\vglass01.dds';p.texture_set.textures[1]=b'textures\\dungeons\\vault\\vglass01_n.dds';p.shader_flags_2.sf_2_z_buffer_write=0;p.shader_flags.sf_glow_map=0
     for slot in range(2,len(p.texture_set.textures)):p.texture_set.textures[slot]=b''
   ap=N.NiAlphaProperty();ap.flags=4845;ps.append(ap)
  st=N.NiStencilProperty();st.flags=19840 if m.get('glass') else 17792;st.draw_mode=3 if m.get('glass') else 1;ps.append(st)
  sh=shape(m['name']+' FC canopy',[[v[0],v[1],v[2]-225.04] for v in m['vertices']],m['triangles'],m['uvs'],ps);sh.set_transform(orig.get_transform())
  if not m.get('glass') and 'inner canopy frame' not in m['name']:
   # Preserve the original animated target object; replace only its geometry.
   if isinstance(orig,N.NiTriStrips):
    data=N.NiTriStripsData();src=sh.data;data.num_vertices=src.num_vertices;data.has_vertices=True;data.vertices.update_size();data.has_normals=True;data.normals.update_size();data.num_uv_sets=1;data.uv_sets.update_size()
    for a,b in zip(data.vertices,src.vertices):a.x,a.y,a.z=b.x,b.y,b.z
    for a,b in zip(data.normals,src.normals):a.x,a.y,a.z=b.x,b.y,b.z
    for a,b in zip(data.uv_sets[0],src.uv_sets[0]):a.u,a.v=b.u,b.v
    data.set_triangles(list(src.get_triangles()));data.update_center_radius();orig.data=data
   else:orig.data=sh.data
   parent.remove_child(sh)

# A matching cabin remains visible through the glass while parked.
interior=node('FCClassicParkedInterior');mapping=N.Matrix44();mapping.set_identity();mapping.m_11=-1;mapping.m_22=-1;mapping.m_42=-100.7;mapping.m_43=-521.24
interior.set_transform(mapping*originals[0].get_transform());parent.add_child(interior)
for sh in fitparts:interior.add_child(sh)
parkout=OUT/'PlayerVertibird.nif'
with parkout.open('wb') as f:pd.write(f)
read(parkout)
if '--install' in sys.argv:
 dest=MOD/'Meshes/PlayerVertibird/Creature/Vertibird/skeleton.nif';backup=OUT/'previous-skeleton.nif'
 if not backup.exists() and dest.exists():shutil.copy2(dest,backup)
 shutil.copy2(output,dest);shutil.copy2(parkout,MOD/'Meshes/PlayerVertibird/PlayerVertibird.nif')
 print('Installed only into development override:',dest)
