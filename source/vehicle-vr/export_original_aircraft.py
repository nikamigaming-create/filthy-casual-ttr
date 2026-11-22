"""Export our original cockpit; no dependency assets are read by this exporter."""
import time,json,math,random,hashlib
from pathlib import Path
from collections import defaultdict
time.clock=time.perf_counter
from pyffi.formats.nif import NifFormat as N
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/original-vertibird'
src=json.loads((OUT/'geometry.json').read_text());contract=json.loads((OUT/'contract.json').read_text());scale=contract['units_per_metre']
tex=OUT/'textures/FilthyCasual/FC27';tex.mkdir(parents=True,exist_ok=True)
mesh=OUT/'meshes/FilthyCasual';mesh.mkdir(parents=True,exist_ok=True)
def srgb(v):return 12.92*v if v<=.0031308 else 1.055*v**(1/2.4)-.055
for name,c in src['materials'].items():
 rng=random.Random(name);im=Image.new('RGBA',(256,256));pixels=[]
 for y in range(256):
  for x in range(256):
   noise=rng.uniform(.86,1.10)
   pixels.append(tuple(round(255*max(0,min(1,srgb(v)*noise))) for v in c)+(255,))
 im.putdata(pixels);im.save(tex/(name+'.dds'))
Image.new('RGBA',(8,8),(128,128,255,50)).save(tex/'flat_n.dds')
def node(name):
 n=N.NiNode();n.name=name.encode();n.flags=14;n.scale=1;n.rotation.set_identity();return n
def color(dst,v):dst.r,dst.g,dst.b=v
def properties(name):
 m=N.NiMaterialProperty();m.name=name.encode();m.alpha=.035 if name=='glass' else 1;m.glossiness=30
 color(m.ambient_color,(1,1,1));color(m.diffuse_color,(1,1,1));color(m.specular_color,(.15,.15,.15));color(m.emissive_color,(.15,.15,.15) if name=='lume' else (0,0,0))
 s=N.BSShaderPPLightingProperty();s.shader_type=1;s.smooth=1;s.shader_flags.sf_specular=1;s.shader_flags.sf_z_buffer_test=1;s.shader_flags_2.sf_2_z_buffer_write=1 if name!='glass' else 0
 t=N.BSShaderTextureSet();t.num_textures=6;t.textures.update_size();t.textures[0]=('textures\\FilthyCasual\\FC27\\'+name+'.dds').encode();t.textures[1]=b'textures\\FilthyCasual\\FC27\\flat_n.dds';s.texture_set=t
 # Shell panels need both faces: the camera can be on either side of the aircraft.
 st=N.NiStencilProperty();st.flags=19840;st.draw_mode=3
 result=[m,s,st]
 if name=='glass':
  a=N.NiAlphaProperty();a.flags=4845;a.threshold=0;result.append(a)
 return result

d=N.Data(version=0x14020007,user_version=11,user_version_2=34);d.header.endian_type=1;d.header.export_info.creator=b'Filthy Casual TTR';d.header.export_info.export_info_1=b'Original cockpit geometry';root=node('FCOriginalAircraft');d.roots=[root]
root.translation.x,root.translation.y,root.translation.z=contract['airframe_origin']
parents={};groups=defaultdict(list)
for m in src['meshes']:groups[m['group'],m['material']].append(m)
reports=[]
def export_parts(group,material,parts,index):
 vertices=[];normals=[];triangles=[];unique={}
 for part in parts:
  remap=[]
  for p,n in zip(part['vertices'],part['normals']):
   key=tuple(round(v,6) for v in p+n)
   if key not in unique:unique[key]=len(vertices);vertices.append([(v-src['pivots'][group][axis])*scale for axis,v in enumerate(p)]);normals.append(n)
   remap.append(unique[key])
  triangles.extend([[remap[i] for i in t] for t in part['triangles']])
 if len(vertices)>60000 or len(triangles)>60000:
  assert len(parts)>1
  mid=len(parts)//2;export_parts(group,material,parts[:mid],index+'a');export_parts(group,material,parts[mid:],index+'b');return
 sh=N.NiTriShape();sh.name=(group+'_'+material+'_'+index).encode();sh.flags=14;sh.scale=1;sh.rotation.set_identity();dat=N.NiTriShapeData();sh.data=dat
 dat.num_vertices=len(vertices);dat.has_vertices=True;dat.vertices.update_size()
 for dst,p in zip(dat.vertices,vertices):dst.x,dst.y,dst.z=p
 dat.has_normals=True;dat.normals.update_size()
 for dst,n in zip(dat.normals,normals):dst.x,dst.y,dst.z=n
 dat.num_uv_sets=1;dat.uv_sets.update_size()
 for dst,p in zip(dat.uv_sets[0],vertices):dst.u=p[0]*.1;dst.v=(p[1]+p[2])*.1
 dat.set_triangles(triangles);dat.update_center_radius();sh.update_tangent_space()
 for prop in properties(material):sh.add_property(prop)
 parents[group].add_child(sh);reports.append({'name':sh.name.decode(),'vertices':len(vertices),'triangles':len(triangles)})
for group in sorted(set(g for g,m in groups)):
 parents[group]=node(group);parents[group].translation.x,parents[group].translation.y,parents[group].translation.z=[v*scale for v in src['pivots'][group]];root.add_child(parents[group])
for (group,material),parts in sorted(groups.items()):export_parts(group,material,parts,'0')
for i,p in enumerate(contract['passengers']):
 seat=node('FCPassenger'+str(i));seat.translation.x,seat.translation.y,seat.translation.z=[v*scale for v in p];root.add_child(seat)
path=mesh/'fc27-original-aircraft.nif'
with path.open('wb') as f:d.write(f)
check=N.Data()
with path.open('rb') as f:check.read(f)
shapes=[b for b in check.get_global_iterator() if isinstance(b,N.NiTriShape)]
assert len(shapes)==len(reports)
for sh in shapes:
 assert 0<sh.data.num_vertices<65536 and 0<sh.data.num_triangles<65536
 for p in sh.data.vertices:assert all(math.isfinite(v) for v in (p.x,p.y,p.z))
 for n in sh.data.normals:assert abs(n.x*n.x+n.y*n.y+n.z*n.z-1)<.002
 for t in sh.data.get_triangles():assert all(0<=i<sh.data.num_vertices for i in t)
(OUT/'nif-validation.json').write_text(json.dumps({'original':True,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'shapes':reports,'roundtrip':True},indent=2))
print(f'Original cockpit exported: {len(shapes)} shapes, {sum(r["triangles"] for r in reports)} triangles, {path.stat().st_size} bytes')
