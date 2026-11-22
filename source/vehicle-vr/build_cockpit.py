"""Original flight deck and cabin. Run with Blender --background --python."""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'vertibird-cockpit';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
COLORS={'paint':(.055,.07,.058),'trim':(.12,.14,.13),'rubber':(.012,.016,.014),'steel':(.25,.27,.24),'fabric':(.105,.12,.085),'ivory':(.67,.68,.54),'black':(.006,.01,.008),'amber':(.8,.27,.025),'green':(.05,.27,.13),'glass':(.18,.28,.25)}
mats={};parts=[]
for name,c in COLORS.items():
 m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*c,.08 if name=='glass' else 1);p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Metallic'].default_value=.65 if name in ('paint','trim','steel') else 0;p.inputs['Roughness'].default_value=.42 if name!='glass' else .05
 if name=='glass':p.inputs['Transmission Weight'].default_value=.9;p.inputs['IOR'].default_value=1.46;p.inputs['Alpha'].default_value=.12;m.surface_render_method='DITHERED'
 if name in ('amber','green'):p.inputs['Emission Color'].default_value=(*c,1);p.inputs['Emission Strength'].default_value=.5
 mats[name]=m

def finish(o,name,mat,bevel=0,group='FCCabin'):
 o.name=name;o.data.materials.append(mats[mat]);o['group']=group
 if bevel:
  bpy.context.view_layer.objects.active=o;mod=o.modifiers.new('Machined edges','BEVEL');mod.width=bevel;mod.segments=3;bpy.ops.object.modifier_apply(modifier=mod.name)
 parts.append(o);return o

def box(name,p,size,mat='paint',bevel=.015,rot=None,group='FCCabin'):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if rot:o.rotation_euler=rot
 return finish(o,name,mat,bevel,group)
def cyl(name,p,r,h,mat='steel',rot=None,group='FCCabin',vertices=32):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=h,location=p);o=bpy.context.object
 if rot:o.rotation_euler=rot
 return finish(o,name,mat,.003,group)
def beam(name,a,b,width=.055,depth=None,mat='paint'):
 a,b=Vector(a),Vector(b);o=box(name,(a+b)*.5,(width,depth or width,(b-a).length),mat,.012);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def mesh(name,v,f,mat):
 me=bpy.data.meshes.new(name);me.from_pydata(v,[],f);me.update();bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free();o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o);return finish(o,name,mat)
def label(text,p,size=.025,rot=None):
 bpy.ops.object.text_add(location=p);o=bpy.context.object;o.data.body=text;o.data.align_x='CENTER';o.data.size=size;o.data.extrude=.0006
 if rot:o.rotation_euler=rot
 bpy.ops.object.convert(target='MESH');return finish(bpy.context.object,'Engraved '+text,'ivory')
# Complete structural shell: floor, side sills and rear pressure bulkhead.
box('Continuous cabin floor',(0,-.55,-.045),(3.05,5.65,.09),'trim',.025)
for x in (-1.5,1.5):
 box('Lower sidewall',(x,-.55,.4),(.09,5.65,.8),'paint',.035)
 beam('Side sill',(x,-3.2,.82),(x,2.2,.82),.07,mat='trim')
box('Rear pressure bulkhead',(0,-3.3,1.05),(3.05,.1,2.1),'paint',.025)
box('Rear hatch',(0,-3.235,1.0),(1.1,.045,1.82),'trim',.05)
for x in (-.46,.46):beam('Hatch locking rail',(x,-3.2,.2),(x,-3.2,1.75),.035,mat='steel')
# Arched frames, canopy roof and full-depth windshield pillars.
arches=[]
for y,z,w in [(-3.2,2.1,1.5),(-1.8,2.1,1.5),(-.4,2.1,1.5),(.75,2.08,1.48)]:
 pts=[(-w,y,.82),(-w,y,1.72),(-w+.2,y,z),(w-.2,y,z),(w,y,1.72),(w,y,.82)];arches.append(pts)
 for a,b in zip(pts,pts[1:]):beam('Formed structural arch',a,b,.065,mat='trim')
for a,b in zip(arches,arches[1:]):
 for j in (1,2,3,4):beam('Longitudinal longeron',a[j],b[j],.055,mat='paint')
 for j in (1,2,3):mesh('Closed overhead skin',[a[j],a[j+1],b[j+1],b[j]],[(0,1,2,3)],'paint')
front=[(-1.25,2.22,.88),(-1.45,.75,1.72),(-1.28,.75,2.08),(1.28,.75,2.08),(1.45,.75,1.72),(1.25,2.22,.88)]
for a,b in zip(front,front[1:]):beam('Windscreen perimeter',a,b,.065,mat='paint')
beam('Windscreen lower sill',front[0],front[-1],.085,mat='trim')
beam('Windscreen center post',(0,2.22,.88),(0,.75,2.08),.055,mat='trim')
for sign in (-1,1):
 pts=[(sign*.035,2.21,.915),(sign*1.20,2.21,.915),(sign*1.40,.76,1.70),(sign*1.25,.76,2.04),(sign*.035,.76,2.04)]
 mesh('Laminated forward windscreen',pts,[(0,1,2,3,4)],'glass')
 for ya,yb in [(-3.15,-1.85),(-1.75,-.45),(-.35,.70)]:
  x=sign*1.47;mesh('Cabin side glazing',[(x,ya,.87),(x,yb,.87),(x,yb,1.68),(x,ya,1.68)],[(0,1,2,3)],'glass')
 # Windshield quarter panels join the front canopy to the sidewall.
 mesh('Forward quarter glazing',[(sign*1.47,.8,.87),(sign*1.25,2.18,.9),(sign*1.45,.76,1.70),(sign*1.47,.70,1.70)],[(0,1,2,3)],'glass')
 beam('Quarter window lower rail',(sign*1.5,.75,.82),(sign*1.25,2.22,.88),.065)
# Upholstered seats with a structural pan, harness and real headrest supports.
def seat(x,y,name,pilot=False):
 box(name+' pedestal',(x,y,.2),(.38,.42,.35),'steel',.025)
 box(name+' pan',(x,y,.39),(.61,.59,.08),'trim',.025)
 for j in range(3):box(name+' cushion',(x,y-.18+j*.18,.46),(.55,.17,.115),'fabric',.038)
 box(name+' back shell',(x,y-.29,.83),(.61,.10,.8),'trim',.04,rot=(math.radians(-7),0,0))
 for j in range(4):box(name+' back cushion',(x,y-.225-j*.017,.59+j*.16),(.54,.12,.15),'fabric',.03)
 for xx in (-.15,.15):beam(name+' headrest rod',(x+xx,y-.32,1.15),(x+xx,y-.32,1.42),.018,mat='steel')
 box(name+' headrest',(x,y-.31,1.39),(.47,.14,.23),'fabric',.055)
 for xx in (-.19,.19):beam(name+' harness',(x+xx,y-.15,.5),(x+xx,y-.17,1.16),.035,.015,'rubber')
 cyl(name+' harness buckle',(x,y-.13,.53),.045,.018,'steel',rot=(math.pi/2,0,0))
 for xx in (-.35,.35):box(name+' armrest',(x+xx,y,.72),(.07,.43,.07),'rubber',.025)
seat(.65,0,'Pilot',True);seat(-.65,0,'Copilot',True)
for y in (-1.2,-2.35):
 for x in (-.92,.92):seat(x,y,'Passenger')
# Shrouded instrument panel: all instruments and switches are mounted to it.
a=math.radians(52);U=Vector((1,0,0));V=Vector((0,math.cos(a),math.sin(a)));N=Vector((0,-math.sin(a),math.cos(a)));P=Vector((0,1.12,.94));rotation=(a,0,0)
def pp(x,y,z=.0):return P+U*x+V*y+N*z
box('Instrument panel body',P,(2.65,.65,.15),'paint',.045,rotation)
beam('Padded glare shield',pp(-1.33,.36,.09),pp(1.33,.36,.09),.1,.16,'rubber')
for x in (-1.22,-.55,.55,1.22):
 for y in (-.27,.27):cyl('Captive panel screw',pp(x,y,.086),.012,.006,'steel',rotation,vertices=16)
def dial(x,y,r,title,value):
 cyl(title+' bezel',pp(x,y,.10),r,.035,'steel',rotation)
 cyl(title+' dial',pp(x,y,.121),r*.88,.008,'black',rotation)
 for i in range(17):
  t=math.radians(-135+i*270/16);p=pp(x+math.sin(t)*r*.73,y+math.cos(t)*r*.73,.128)
  o=box(title+' index',p,(.006,.018,.002),'ivory',.001,rotation);o.rotation_euler.rotate_axis('Z',-t)
 t=math.radians(-135+270*value);end=pp(x+math.sin(t)*r*.62,y+math.cos(t)*r*.62,.132)
 beam(title+' needle',pp(x,y,.132),end,.008,.004,'ivory');cyl(title+' pivot',pp(x,y,.136),.012,.009,'steel',rotation)
 label(title,pp(x,y-r*.35,.132),.022,rotation)
for x,lower,upper in [(-1.03,'OIL','ALT'),(-.72,'TEMP','ROTOR'),(.72,'CLIMB','AIRSPEED'),(1.03,'FUEL','RPM')]:
 for y,title in [(-.13,lower),(.17,upper)]:dial(x,y,.122,title,.3+.2*(x+1)/2)
box('Navigation display enclosure',pp(0,.045,.13),(.58,.4,.13),'trim',.025,rotation)
box('Recessed navigation glass',pp(0,.045,.20),(.49,.31,.012),'green',.015,rotation)
label('NAV / STANDBY',pp(0,.045,.211),.027,rotation)
label('FLIGHT SYSTEMS',pp(0,.255,.1),.027,rotation)
# Central pedestal with tactile switches; avoid loose labels and floating detail.
box('Center control pedestal',(0,.62,.51),(.40,.72,.65),'paint',.045)
for row in range(3):
 for x in (-.10,0,.10):
  z=.85;y=.4+row*.16;cyl('Toggle mounting collar',(x,y,z),.018,.012,'steel');beam('Guarded toggle',(x,y,z),(x,y-.025,z+.055),.012,mat='steel')
for x in (-.16,.16):beam('Toggle protective rail',(x,.29,.89),(x,.80,.89),.018,mat='amber')
# Pilot cyclic and collective remain separate named rigid parts for interaction.
for x in (-.65,.65):
 cyl('Cyclic mounting boot',(x,.4,.24),.085,.23,'rubber')
 beam('Cyclic stem',(x,.4,.34),(x,.48,.68),.026,mat='steel')
 box('Cyclic handgrip',(x,.48,.75),(.065,.07,.17),'rubber',.022,group='FCCyclicRight' if x>0 else 'FCCyclicLeft')
 cyl('Cyclic thumb switch',(x,.48,.845),.018,.012,'amber')
# Collective and anti-torque pedals, mounted within reach of each seat.
for x in (-.65,.65):
 cx=x-.29
 cyl('Collective pivot',(cx,-.08,.40),.05,.08,'steel',rot=(0,math.pi/2,0))
 beam('Collective arm',(cx,-.08,.40),(cx,.30,.65),.032,mat='steel')
 box('Collective grip',(cx,.30,.65),(.065,.19,.065),'rubber',.02)
 for dx in (-.13,.13):
  beam('Pedal linkage',(x+dx,.78,.07),(x+dx,.85,.18),.025,mat='steel')
  box('Anti torque pedal',(x+dx,.85,.20),(.20,.12,.04),'steel',.014,rot=(math.radians(20),0,0))
# Rivets, cable conduits and overhead practical lighting complete the shell.
for x in (-1.43,1.43):
 for y in [-3,-2.5,-2,-1.5,-1,-.5,0,.5,1,1.5]:cyl('Flush sill rivet',(x,y,.835),.008,.004,'steel',vertices=12)
 beam('Protected cable conduit',(x,-3.1,.28),(x,1.45,.28),.025,mat='rubber')
for y in (-2.5,-1,.3):
 box('Overhead lamp housing',(0,y,2.045),(.32,.16,.04),'trim',.02)
 box('Overhead lamp diffuser',(0,y,2.02),(.27,.115,.012),'ivory',.012)
# Export actual evaluated triangles with authored normals, no camera or lights.
bpy.context.view_layer.update();meshes=[]
for o in parts:
 me=o.data;me.calc_loop_triangles();vs=[];ns=[];ts=[]
 for tri in me.loop_triangles:
  xyz=[o.matrix_world@me.vertices[i].co for i in tri.vertices]
  if (xyz[1]-xyz[0]).cross(xyz[2]-xyz[0]).length<1e-10:continue
  base=len(vs);vs.extend([list(v) for v in xyz]);ns.extend([list((o.matrix_world.to_3x3()@me.corner_normals[j].vector).normalized()) for j in tri.loops]);ts.append([base,base+1,base+2])
 meshes.append({'name':o.name,'group':o['group'],'material':me.materials[0].name,'vertices':vs,'normals':ns,'triangles':ts})
(OUT/'geometry.json').write_text(json.dumps({'materials':COLORS,'meshes':meshes}))
(OUT/'contract.json').write_text(json.dumps({'units':'metres','airframe_origin':[0,120,405],'units_per_metre':69.99,'pilot_eye':[.65,0,1.25],'pilot_anchor':[45.4935,248,415.5],'passengers':[[-.92,-1.2,0],[.92,-1.2,0],[-.92,-2.35,0],[.92,-2.35,0]],'cyclic_right':[.65,.48,.75]},indent=2))
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32;scene.render.resolution_x=1280;scene.render.resolution_y=960;scene.render.resolution_percentage=100;scene.world.color=(.3,.35,.4)
scene.view_settings.view_transform='AgX'
for p,power,size in [((0,4,4),1800,5),((-4,0,3),1200,4),((0,-2,1.9),80,2)]:
 bpy.ops.object.light_add(type='AREA',location=p);o=bpy.context.object;o.data.energy=power;o.data.size=size;o.rotation_euler=(Vector((0,.5,.7))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(.65,0,1.25));cam=bpy.context.object;cam.data.lens=20;scene.camera=cam
for name,pos,target in [('pilot-view',(.65,0,1.25),(.65,2,.98)),('cabin-view',(0,1.1,1.6),(0,-2,.8)),('cockpit-overview',(4,5,3),(0,.2,.8))]:
 cam.location=pos;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(OUT/(name+'.png'));bpy.ops.render.render(write_still=True)
bpy.context.preferences.filepaths.save_version=0;bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'filthy-casual-vertibird.blend'))
print('Original cockpit source, geometry, attachment contract and previews written')
