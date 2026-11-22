"""Build an original collision-bearing carrier for empty detachable gun parts.
No game or third-party assets are read or redistributed by this generator.
The live addon supplies the exact locally owned part's appearance.
"""
from pathlib import Path
import math,struct,time
time.clock=time.perf_counter
from pyffi.formats.nif import NifFormat as N
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/reload-parts/mod';M=OUT/'meshes/FilthyCasual';T=OUT/'textures/FilthyCasual'
M.mkdir(parents=True,exist_ok=True);T.mkdir(parents=True,exist_ok=True)
Image.new('RGBA',(4,4),(87,88,84,255)).save(T/'reload-carrier.dds')
d=N.Data(version=0x14020007,user_version=11,user_version_2=34)
d.header.endian_type=1
r=N.BSFadeNode();r.name=b'FCReloadPart';r.flags=14;r.scale=1;r.rotation.set_identity();d.roots=[r]
flags=N.BSXFlags();flags.name=b'BSX';flags.integer_data=66;r.add_extra_data(flags)
col=N.bhkCollisionObject();col.target=r;col.flags=129;r.collision_object=col
body=N.bhkRigidBody();col.body=body
body.havok_col_filter.layer=4;body.havok_col_filter_copy.layer=4
body.collision_response=1;body.motion_system=4;body.quality_type=3;body.deactivator_type=2;body.solver_deactivation=2
body.mass=.15;body.friction=.65;body.restitution=.12;body.linear_damping=.1;body.angular_damping=.1
shape=N.bhkBoxShape();body.shape=shape;shape.radius=.02;shape.material.material=5
half=(.8,1.4,3.5)
shape.dimensions.x,shape.dimensions.y,shape.dimensions.z=[v/7 for v in half]
for axis,i in zip(('11','22','33'),range(3)):
 others=[half[j]/7 for j in range(3) if j!=i]
 setattr(body.inertia,'m_'+axis,body.mass*sum(v*v for v in others)/3)
geom=N.NiTriShape();geom.name=b'FCReloadCarrierVisual';geom.flags=14;geom.scale=1;geom.rotation.set_identity();r.add_child(geom)
a=N.NiTriShapeData();geom.data=a
verts=[(sx*half[0],sy*half[1],sz*half[2]) for sx,sy,sz in ((-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1))]
a.num_vertices=8;a.has_vertices=True;a.vertices.update_size();a.has_normals=True;a.normals.update_size();a.num_uv_sets=1;a.uv_sets.update_size()
for v,p,n,uv in zip(a.vertices,verts,a.normals,a.uv_sets[0]):
 v.x,v.y,v.z=p;ln=math.sqrt(sum(t*t for t in p));n.x,n.y,n.z=[t/ln for t in p];uv.u,uv.v=0,0
a.set_triangles([(0,2,1),(0,3,2),(4,5,6),(4,6,7),(0,1,5),(0,5,4),(1,2,6),(1,6,5),(2,3,7),(2,7,6),(3,0,4),(3,4,7)]);a.update_center_radius();geom.update_tangent_space()
mat=N.NiMaterialProperty();mat.alpha=1;mat.glossiness=15
for dst in (mat.ambient_color,mat.diffuse_color):dst.r=dst.g=dst.b=1
shader=N.BSShaderPPLightingProperty();shader.shader_type=1;shader.shader_flags.sf_z_buffer_test=1;shader.shader_flags_2.sf_2_z_buffer_write=1
tex=N.BSShaderTextureSet();tex.num_textures=6;tex.textures.update_size();tex.textures[0]=b'textures\\FilthyCasual\\reload-carrier.dds';shader.texture_set=tex
geom.add_property(mat);geom.add_property(shader)
path=M/'reload-carrier.nif'
with path.open('wb') as f:d.write(f)
check=N.Data()
with path.open('rb') as f:check.read(f)
assert any(isinstance(x,N.bhkRigidBody) and x.mass>0 and x.motion_system==4 for x in check.get_global_iterator())
def sub(t,b):return t.encode()+struct.pack('<H',len(b))+b
def z(s):return s.encode()+b'\0'
def rec(t,i,b):return struct.pack('<4sIIIIHH',t.encode(),len(b),0,i,0,15,0)+b
h=sub('HEDR',struct.pack('<fII',1.34,1,0x801))+sub('CNAM',z('Nikami'))+sub('MAST',z('FalloutNV.esm'))+sub('DATA',bytes(8))
b=sub('EDID',z('FCEmptyReloadPart'))+sub('OBND',struct.pack('<6h',-1,-2,-4,1,2,4))+sub('FULL',z('Empty weapon part'))+sub('MODL',z('FilthyCasual\\reload-carrier.nif'))+sub('DATA',struct.pack('<If',0,.15))
record=rec('MISC',0x01000800,b)
group=struct.pack('<4sI4siHHHH',b'GRUP',len(record)+24,b'MISC',0,0,0,0,0)+record
(OUT/'Filthy Casual Reload Parts.esp').write_bytes(rec('TES4',0,h)+group)
print('Built original empty-part carrier with native collision; runtime acceptance pending.')

# Native NiStream's Havok writer includes runtime-only fields. Persist our own
# canonical serialized Cinfo instead, preserving only the linked shape index.
raw=path.read_bytes();off=check.header.get_size(data=check);payloads={}
for block,size in zip(check.blocks,check.header.block_size):
 if type(block).__name__ in ('bhkBoxShape','bhkRigidBody'):payloads[type(block).__name__]=raw[off:off+size]
 off+=size
lines=['#pragma once','// Generated solely from our original carrier by tools/build_reload_carrier.py.','namespace reload_carrier_data {']
for key,name in [('bhkBoxShape','box'),('bhkRigidBody','body')]:
 data=payloads[key];lines.append('inline constexpr unsigned char '+name+'[] = {')
 lines.extend(' '+','.join('0x%02X'%b for b in data[i:i+16])+',' for i in range(0,len(data),16))
 lines.append('};')
lines.append('}')
(ROOT/'reload_carrier_data.hpp').write_text('\n'.join(lines)+'\n')
