"""Original wrist controls: run with headless Blender. Authored in centimetres.
No game geometry, textures, fonts or third-party assets are imported.
"""
import bpy, bmesh, math, json, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'wrist-controls'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
random.seed(417)
COLORS={'olive':(.145,.175,.095),'brass':(.41,.31,.14),'ivory':(.78,.74,.56),
        'dark':(.035,.041,.031),'amber':(.95,.43,.045),'steel':(.24,.25,.21)}
materials={}
for name,rgb in COLORS.items():
 m=bpy.data.materials.new(name);m.use_nodes=True
 p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
 p.inputs['Base Color'].default_value=(*rgb,1)
 p.inputs['Metallic'].default_value=.7 if name in ('brass','steel','olive') else .15
 p.inputs['Roughness'].default_value=.55
 m.diffuse_color=(*rgb,1);materials[name]=m
objects=[]
def finish(o,name,mat,group='FC_Plate',bevel=0):
 o.name=name;o.data.materials.append(materials[mat]);o['group']=group
 if bevel:
  mod=o.modifiers.new('Machined edges','BEVEL');mod.width=bevel;mod.segments=1
  bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
 objects.append(o);return o
def box(name,loc,size,mat,group='FC_Plate',bevel=.025,angle=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.rotation_euler.z=angle
 return finish(o,name,mat,group,bevel)
def disc(name,x,y,z,r,h,mat,group,vertices=32):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=h,location=(x,y,z))
 return finish(bpy.context.object,name,mat,group,.035)
def polygon(name,points,z,depth,mat,group):
 n=len(points);verts=[(x,y,z) for x,y in points]+[(x,y,z+depth) for x,y in points]
 faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
 faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
 o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o)
 return finish(o,name,mat,group,.012)
def stroke(name,points,z,width,mat,group):
 for i,(a,b) in enumerate(zip(points,points[1:])):
  dx=b[0]-a[0];dy=b[1]-a[1]
  box(name+str(i),((a[0]+b[0])/2,(a[1]+b[1])/2,z+i*.012),
   (math.hypot(dx,dy)+width,width,.055),mat,group,.012,math.atan2(dy,dx))
box('Back gasket',(0,0,-.06),(6.8,4.8,.22),'dark',bevel=.18)
box('Cast brass edge',(0,0,.05),(7,5,.27),'brass',bevel=.23)
box('Olive enamel plate',(0,0,.17),(6.83,4.83,.18),'olive',bevel=.22)
# Simple original wrap strap: forearm axis Y, anatomical center Z=-2.6cm.
verts=[];faces=[];nstrap=40
for side in (-.78,.78):
 for inner in (False,True):
  for i in range(nstrap):
   a=i*math.tau/nstrap;rx=3.16 if inner else 3.34;rz=2.43 if inner else 2.61
   verts.append((rx*math.cos(a),side,-2.6+rz*math.sin(a)))
for i in range(nstrap):
 j=(i+1)%nstrap
 faces.extend([(i,j,nstrap+j,nstrap+i),(2*nstrap+i,3*nstrap+i,3*nstrap+j,2*nstrap+j),
  (i,2*nstrap+i,2*nstrap+j,j),(nstrap+i,nstrap+j,3*nstrap+j,3*nstrap+i)])
mesh=bpy.data.meshes.new('Original wrap strap');mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('Dark wrist strap',mesh);bpy.context.collection.objects.link(o);finish(o,o.name,'dark')
for x in (-3.08,3.08):
 box('Strap anchor',(x,0,-.10),(.32,1.86,.30),'brass',bevel=.05)
for i in range(24):
 side=i%4;t=random.uniform(-1,1)
 x=3.28*(1 if side==0 else -1) if side<2 else t*3.15
 y=t*2.15 if side<2 else 2.29*(1 if side==2 else -1)
 box('Edge wear %02d'%i,(x,y,.266),(random.uniform(.07,.20),.018,.006),'brass',bevel=0,angle=random.uniform(-.8,.8))
for x in (-3.10,3.10):
 for y in (-2.10,2.10):
  disc('Slotted screw',x,y,.28,.12,.09,'steel','FC_Plate',16)
  box('Screw slot',(x,y,.329),(.17,.026,.008),'dark',bevel=0,angle=.4)
CENTERS={'FC_Weapons':(-1.65,1,.48),'FC_Consumables':(1.65,1,.48),
 'FC_Reload_Auto':(-1.65,-1.30,.46),'FC_Reload_Manual':(-1.65,-1.30,.46),'FC_Settings':(1.65,-1.30,.46)}
for group,(x,y,z) in CENTERS.items():
 r=1.2 if group in ('FC_Weapons','FC_Consumables') else .9
 disc(group+' bezel',x,y,.34,r,.18,'brass',group)
 disc(group+' well',x,y,.44,r-.10,.12,'dark',group)
 disc(group+' enamel',x,y,.478,r-.19,.065,'olive' if r>1 else 'dark',group)
 for j in range(16):
  a=j*math.tau/16
  box(group+' knurl%02d'%j,(x+(r-.025)*math.cos(a),y+(r-.025)*math.sin(a),.35),(.10,.045,.14),'brass',group,0,a)
x,y,_=CENTERS['FC_Weapons']
polygon('Pistol relief',[(x-.72,y+.35),(x+.65,y+.35),(x+.65,y+.09),(x+.05,y+.09),
 (x-.08,y-.10),(x+.03,y-.58),(x-.35,y-.58),(x-.55,y+.08),(x-.72,y+.08)],.518,.065,'ivory','FC_Weapons')
stroke('Trigger guard',[(x-.32,y+.07),(x-.19,y-.27),(x+.22,y-.27),(x+.23,y+.07)],.57,.065,'ivory','FC_Weapons')
x,y,_=CENTERS['FC_Consumables']
box('Supply tin',(x,y,.56),(1.16,.86,.08),'ivory','FC_Consumables',.08)
stroke('Tin handle',[(x-.25,y+.45),(x-.25,y+.62),(x+.25,y+.62),(x+.25,y+.45)],.56,.09,'ivory','FC_Consumables')
box('Supply cross horizontal',(x,y,.61),(.60,.18,.035),'olive','FC_Consumables',.008)
box('Supply cross vertical',(x,y,.612),(.18,.60,.039),'olive','FC_Consumables',.008)
x,y,_=CENTERS['FC_Reload_Auto']
stroke('Auto A',[(x-.35,y-.38),(x,y+.40),(x+.35,y-.38)],.56,.13,'amber','FC_Reload_Auto')
stroke('Auto A bar',[(x-.22,y-.10),(x+.22,y-.10)],.59,.12,'amber','FC_Reload_Auto')
stroke('Manual M',[(x-.36,y-.38),(x-.36,y+.38),(x,y-.06),(x+.36,y+.38),(x+.36,y-.38)],.56,.12,'ivory','FC_Reload_Manual')
x,y,_=CENTERS['FC_Settings'];n=64;verts=[]
for z in (.525,.59):
 for inner in (False,True):
  for i in range(n):
   a=i*math.tau/n;r=.30 if inner else (.64 if i%8 in (1,2,3,4) else .53)
   verts.append((x+r*math.cos(a),y+r*math.sin(a),z))
faces=[]
for i in range(n):
 j=(i+1)%n
 faces.extend([(i,j,n+j,n+i),(2*n+i,3*n+i,3*n+j,2*n+j),(i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)])
mesh=bpy.data.meshes.new('Gear mesh');mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('Settings gear relief',mesh);bpy.context.collection.objects.link(o);finish(o,o.name,'ivory','FC_Settings')
for ob in objects:
 bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free()
bpy.context.view_layer.update()
meshes=[]
for ob in objects:
 mesh=ob.data;mesh.calc_loop_triangles();matrix=ob.matrix_world
 vertices=[];normals=[];triangles=[]
 for tri in mesh.loop_triangles:
  xyz=[matrix@mesh.vertices[i].co for i in tri.vertices]
  norm=(xyz[1]-xyz[0]).cross(xyz[2]-xyz[0])
  if norm.length<1e-9:continue
  norm.normalize();start=len(vertices)
  vertices.extend([list(v) for v in xyz])
  normals.extend([list(norm)]*3);triangles.append([start,start+1,start+2])
 meshes.append({'name':ob.name,'group':ob['group'],'material':ob.data.materials[0].name,
  'vertices_cm':vertices,'normals':normals,'triangles':triangles})
(OUT/'geometry.json').write_text(json.dumps({'materials':COLORS,'meshes':meshes}),encoding='utf-8')
factor=.6999125
contract={'root':'FC_WristControls','units_per_metre':69.99125,'units_per_cm':factor,
 'axes':{'x':'across wrist','y':'toward elbow','z':'outward from palm-facing surface'},
 'plate_dimensions_cm':[7,5,.6],'buttons':[]}
for name,c in CENTERS.items():
 contract['buttons'].append({'node':name,'center_cm':c,'center_game_units':[v*factor for v in c],
  'radius_cm':1.2 if name in ('FC_Weapons','FC_Consumables') else .9})
(OUT/'attachment.json').write_text(json.dumps(contract,indent=2),encoding='utf-8')
for ob in objects:ob.hide_render=ob['group']=='FC_Reload_Manual';ob.hide_set(ob.hide_render)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
scene.render.engine='CYCLES';scene.cycles.samples=32
scene.render.resolution_x=1400;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world.color=(.25,.25,.25)
bpy.ops.object.camera_add(location=(6,-8,15));camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,.2))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO';camera.data.ortho_scale=9.9;scene.camera=camera
for pos,power,size in [((-4,-3,9),1700,7),((5,4,6),1200,5)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size
 o.rotation_euler=(-o.location).to_track_quat('-Z','Y').to_euler()
scene.render.image_settings.file_format='PNG';scene.render.film_transparent=True
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'filthy-casual-wrist.blend'))
scene.render.filepath=str(OUT/'preview-auto.png');bpy.ops.render.render(write_still=True)
for ob in objects:ob.hide_render=ob['group']=='FC_Reload_Auto';ob.hide_set(ob.hide_render)
scene.render.filepath=str(OUT/'preview-manual.png');bpy.ops.render.render(write_still=True)
print('Wrist geometry exported:',len(meshes),'parts')
