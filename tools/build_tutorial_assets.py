"""Original 3D tutorial lines/cards. Reads no game or third-party assets."""
from pathlib import Path
import math,time,re
time.clock=time.perf_counter
from pyffi.formats.nif import NifFormat as N
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];O=R/'build/tutorial-assets';M=O/'meshes/FilthyCasual';T=O/'textures/FilthyCasual/Tutorial'
M.mkdir(parents=True,exist_ok=True);T.mkdir(parents=True,exist_ok=True)
texts={
 'pump-ready':('READY TO FIRE','Trigger fires the chambered round','Cycle the pump after each shot'),
 'charger-guide':('OPEN THE ACTION','Pull the slide with the off-hand trigger','Tap A/X to lock it back; grip a loaded clip'),
 'insert-charger':('FEED THE CHARGER CLIP','Align the clip above the highlighted guide','Lower into place, then release grip'),
 'charger-ready':('READY TO FIRE','Trigger fires the chambered round','This weapon reloads through the top guide'),
 'choose-weapon':('RELOAD FIELD GUIDE','Draw a weapon to begin','Triple-tap Menu to close'),
 'unmapped':('NATIVE WEAPON','No measured reload guide for this item','Normal controls still work'),
 'automatic':('AUTOMATIC RELOAD','Trigger fires | A/X reloads','Tap Menu for manual reloading'),
 'ready':('READY TO FIRE','Trigger fires | A/X ejects ammo','Green outlines mark moving parts'),
 'remove-magazine':('REMOVE THE EMPTY MAGAZINE','Press A/X with the gun hand','Or grip the highlighted magazine'),
 'fetch-ammo':('GET AMMO FROM THE POUCH','Look down and grip a spare','Keep holding it as you lift'),
 'insert-magazine':('INSERT THE MAGAZINE','Align it with the highlighted well','Push into place, then release grip'),
 'cycle-action':('CYCLE THE ACTION','Hold the other trigger at the highlight','Pull the slide or bolt, then release'),
 'load-rounds':('LOAD THE HIGHLIGHTED PORT','Grip ammunition from the pouch','Bring it to the highlighted loading port'),
 'moving-part':('FOLLOW THE LOADING PATH','Keep the part lined up with the opening','Finish the motion, then release'),
 'cylinder-guide':('REVOLVER CYLINDER','Hold A/X and flick to open the cylinder','Grip a loader from the pouch; insert, then close'),
 'gate-guide':('REVOLVER LOADING GATE','Hold A/X and flick to open the gate','Grip rounds from the pouch; load, then close'),
 'break-guide':('BREAK-ACTION RELOAD','Hold A/X and flick to open the breech','Grip shells from the pouch; load, then close'),
 'pump-guide':('PUMP THE ACTION','Hold A/X on the gun hand','Grip the fore-end; pull back, then push forward'),
 'load-tube':('LOAD THE SHOTGUN TUBE','Grip shells from the pouch','Insert at the highlighted loading port'),
 'lever-guide':('CYCLE THE LEVER','Hold A/X on the gun hand','Grip the action; pull back, then return'),
 'bolt-guide':('CYCLE THE BOLT','Hold the off-hand trigger at the bolt','Pull back, return, then release'),
 'charging-guide':('PULL THE CHARGING HANDLE','Hold the off-hand trigger at the handle','Pull back, then release'),
 'remove-cell':('REMOVE THE EMPTY CELL','Press A/X with the gun hand','Or grip the highlighted cell'),
 'insert-cell':('INSERT THE ENERGY CELL','Align it with the highlighted socket','Push into place, then release grip'),
}
# The runtime and generated assets must agree, including every special lesson.
header=(R/'reload_tutorial.hpp').read_text()
expected=set(re.findall(r'"([^"\n]+)"',header.split('cardNames[]={',1)[1].split('};',1)[0]))
assert set(texts)==expected,(set(texts)-expected,expected-set(texts))
def fitted_font(text, bold, size):
 path='C:/Windows/Fonts/arialbd.ttf' if bold else 'C:/Windows/Fonts/arial.ttf'
 while size>=32:
  f=ImageFont.truetype(path,size)
  box=f.getbbox(text)
  if box[2]-box[0]<=944:return f
  size-=1
 raise ValueError('Tutorial text exceeds readable card width: '+text)
for name,lines in texts.items():
 im=Image.new('RGBA',(1024,384),(10,22,18,255));d=ImageDraw.Draw(im)
 d.rounded_rectangle((3,3,1020,380),radius=15,outline=(118,245,157),width=4)
 for i,(text,y,c) in enumerate(zip(lines,(35,153,252),((149,255,175),(239,235,204),(239,235,204)))):
  f=fitted_font(text,i==0,56 if i==0 else 48)
  box=d.textbbox((512,y),text,anchor='mt',font=f)
  assert box[0]>=20 and box[2]<=1004 and box[3]<364,(name,box)
  d.text((512,y),text,anchor='mt',font=f,fill=c)
 im.save(T/(name+'.dds'))
Image.new('RGBA',(4,4),(86,255,120,255)).save(T/'line.dds')
Image.new('RGBA',(4,4),(128,128,255,255)).save(T/'flat_n.dds')
def node(name):
 n=N.NiNode();n.name=name.encode();n.flags=14;n.scale=1;n.rotation.set_identity();return n
def shape(name,verts,tris,uv,texture):
 s=N.NiTriShape();s.name=name.encode();s.flags=14;s.scale=1;s.rotation.set_identity()
 a=N.NiTriShapeData();s.data=a;a.num_vertices=len(verts);a.has_vertices=True;a.vertices.update_size();a.has_normals=True;a.normals.update_size();a.num_uv_sets=1;a.uv_sets.update_size()
 for p,v,n,t,u in zip(a.vertices,verts,a.normals,a.uv_sets[0],uv):p.x,p.y,p.z=v;n.x,n.y,n.z=0,0,1;t.u,t.v=u
 a.set_triangles(tris);a.update_center_radius();s.update_tangent_space()
 mat=N.NiMaterialProperty();mat.alpha=1
 for color in (mat.diffuse_color,mat.ambient_color,mat.specular_color):color.r=color.g=color.b=0
 mat.emissive_color.r=mat.emissive_color.g=mat.emissive_color.b=1
 mat.glossiness=0
 # Unlit UI shader: avoid scene/muzzle lights bleaching the instruction card.
 # Niftools BSShaderType SHADER_NOLIGHTING=33; use its matching property class.
 shader=N.BSShaderNoLightingProperty();shader.shader_type=33
 shader.shader_flags.sf_z_buffer_test=1;shader.shader_flags_2.sf_2_z_buffer_write=1;shader.shader_flags_2.sf_2_no_fade=1
 shader.file_name=('textures\\FilthyCasual\\Tutorial\\'+texture+'.dds').encode()
 shader.falloff_start_opacity=shader.falloff_stop_opacity=1

 s.add_property(mat);s.add_property(shader);return s
f=N.Data(version=0x14020007,user_version=11,user_version_2=34);f.header.endian_type=1;f.header.export_info.creator=b'Filthy Casual TTR';root=node('FC_Tutorial');f.roots=[root]
# Unit line along local X. Each edge gets its own transform and properties.
for i in range(36):
 parent=node('FCT_Edge'+str(i));root.add_child(parent)
 verts=[(x,y,z) for x in (0.,1.) for y,z in ((-.007,-.007),(.007,-.007),(.007,.007),(-.007,.007))]
 tris=[(0,2,1),(0,3,2),(4,5,6),(4,6,7),(0,1,5),(0,5,4),(1,2,6),(1,6,5),(2,3,7),(2,7,6),(3,0,4),(3,4,7)]
 parent.add_child(shape('FCT_Line'+str(i),verts,tris,[(0,0)]*8,'line'))
# 40cm x15cm at native70units/metre. Card faces local positive Z.
for name in texts:
 parent=node('FCT_Card_'+name);root.add_child(parent)
 parent.add_child(shape('FCT_Text_'+name,[(-14,-5.25,0),(14,-5.25,0),(14,5.25,0),(-14,5.25,0)],[(0,1,2),(0,2,3),(2,1,0),(3,2,0)],[(0,1),(1,1),(1,0),(0,0)],name))
p=M/'tutorial.nif'
with p.open('wb') as stream:f.write(stream)
c=N.Data()
with p.open('rb') as stream:c.read(stream)
assert sum(isinstance(b,N.NiTriShape) for b in c.get_global_iterator())==36+len(texts)
assert not any('bhk' in type(b).__name__ for b in c.get_global_iterator())
print(f'Built 36 original 3D highlight edges and {len(texts)} instruction cards; no collision or game assets.')
