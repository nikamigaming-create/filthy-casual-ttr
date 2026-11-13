"""Original FC-27 twin-rotor aircraft. No retail meshes or textures are read.
Blender headless source; coordinates metres, +Y forward, +Z up.
"""
import bpy,math,json,random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/original-vertibird';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
COLORS={'olive':(.045,.075,.057),'edge':(.10,.14,.115),'dark':(.035,.047,.043),'steel':(.32,.36,.35),'brass':(.43,.27,.09),'rubber':(.018,.021,.019),'leather':(.12,.16,.115),'ivory':(.73,.70,.53),'red':(.39,.045,.025),'amber':(.85,.38,.055),'lume':(.33,.62,.40),'glass':(.25,.40,.40)}
mats={};parts=[];pivots={'FCAirframe':(0,0,0),'FCRotorLeft':(-4.0,-.65,2.58),'FCRotorRight':(4.0,-.65,2.58),'FCGearNose':(0,2.2,-.28),'FCGearLeft':(-1.0,-1.35,-.25),'FCGearRight':(1.0,-1.35,-.25),'FCCyclicLeft':(-.58,1.60,.25),'FCCyclicRight':(.58,1.60,.25)}
for name,c in COLORS.items():
 m=bpy.data.materials.new(name);m.diffuse_color=(*c,.10 if name=='glass' else 1);m.use_nodes=True;n=m.node_tree.nodes;p=n.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Roughness'].default_value=.43;p.inputs['Metallic'].default_value=.7 if name in ['steel','brass'] else .32 if name in ['olive','edge','dark'] else 0
 if name=='glass':p.inputs['Transmission Weight'].default_value=.65;p.inputs['Roughness'].default_value=.09;p.inputs['Alpha'].default_value=.20;m.surface_render_method='DITHERED'
 elif name in ['amber','lume']:p.inputs['Emission Color'].default_value=(*c,1);p.inputs['Emission Strength'].default_value=.65
 else:
  tex=n.new('ShaderNodeTexNoise');tex.inputs['Scale'].default_value=65 if name in ['rubber','leather'] else 95;tex.inputs['Detail'].default_value=2
  bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.18;bump.inputs['Distance'].default_value=.008;m.node_tree.links.new(tex.outputs['Fac'],bump.inputs['Height']);m.node_tree.links.new(bump.outputs['Normal'],p.inputs['Normal'])
 mats[name]=m

def finish(o,name,mat,bevel=0,group='FCAirframe',smooth=False):
 o.name=name;o.data.materials.append(mats[mat]);o['group']=group
 if bevel:
  bpy.context.view_layer.objects.active=o;mod=o.modifiers.new('Edge radius','BEVEL');mod.width=bevel;mod.segments=2;bpy.ops.object.modifier_apply(modifier=mod.name)
 if smooth:
  for f in o.data.polygons:f.use_smooth=True
 parts.append(o);return o

def box(name,p,size,mat='olive',bevel=.015,rot=None,group='FCAirframe'):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if rot:o.rotation_euler=rot
 return finish(o,name,mat,bevel,group)

def cyl(name,p,r,h,mat='steel',rot=None,group='FCAirframe',verts=24):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=h,location=p);o=bpy.context.object
 if rot:o.rotation_euler=rot
 return finish(o,name,mat,.004,group,True)

def tube(name,pts,r=.025,mat='edge',group='FCAirframe'):
 curve=bpy.data.curves.new(name,'CURVE');curve.dimensions='3D';curve.resolution_u=2;curve.bevel_depth=r;curve.bevel_resolution=2;sp=curve.splines.new('POLY');sp.points.add(len(pts)-1)
 for v,p in zip(sp.points,pts):v.co=(*p,1)
 o=bpy.data.objects.new(name,curve);bpy.context.collection.objects.link(o);bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.convert(target='MESH');o=bpy.context.object;o.select_set(False);return finish(o,name,mat,0,group,True)

def mesh(name,vs,faces,mat,group='FCAirframe',solid=0):
 me=bpy.data.meshes.new(name);me.from_pydata(vs,[],faces);me.update();o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o)
 if solid:
  bpy.context.view_layer.objects.active=o;mo=o.modifiers.new('Shell thickness','SOLIDIFY');mo.thickness=solid;bpy.ops.object.modifier_apply(modifier=mo.name)
 return finish(o,name,mat,0,group,True)

def label(name,text,p,size=.05,rot=(math.pi/2,0,0),mat='ivory'):
 bpy.ops.object.text_add(location=p,rotation=rot);o=bpy.context.object;o.data.body=text;o.data.size=size;o.data.align_x='CENTER';o.data.extrude=.0005;o.data.resolution_u=2;bpy.ops.object.convert(target='MESH');return finish(o,name,mat)

def torus(name,p,major,minor,mat='rubber',rot=None,group='FCAirframe'):
 bpy.ops.mesh.primitive_torus_add(major_segments=32,minor_segments=10,location=p,major_radius=major,minor_radius=minor);o=bpy.context.object
 if rot:o.rotation_euler=rot
 return finish(o,name,mat,0,group,True)

# Rounded monocoque body. Front upper shell is glazing; cabin side doors are open.
sections=[(-5.5,.12,.78,.17),(-4.5,.30,.78,.34),(-3.1,.72,.77,.70),(-2.4,1.08,.80,1.02),(-1.7,1.25,.82,1.13),(-.4,1.30,.84,1.16),(.65,1.28,.84,1.16),(1.35,1.20,.84,1.12),(2.05,1.08,.76,1.06),(2.65,.85,.62,.90),(3.15,.52,.46,.60),(3.48,.08,.34,.20)]
N=32
rings=[[(rx*math.sin(i*2*math.pi/N),y,cz+rz*math.cos(i*2*math.pi/N)) for i in range(N)] for y,rx,cz,rz in sections]
for j in range(len(sections)-1):
 vs=rings[j]+rings[j+1];opaque=[];glass=[]
 for i in range(N):
  k=(i+1)%N;theta=(i+.5)*2*math.pi/N;top=math.cos(theta)>-.24
  doorway=j in [3,4] and abs(math.sin(theta))>.77
  if doorway:continue
  face=(i,k,N+k,N+i)
  if j>=6 and top and j<10:glass.append(face)
  else:opaque.append(face)
 if opaque:mesh('Riveted hull segment '+str(j),vs,opaque,'olive',solid=.025)
 if glass:mesh('Canopy laminated segment '+str(j),vs,glass,'glass',solid=.008)
 # structural circumferential frames and subtle exterior panel seams
 if j in [2,3,5,6,7,8,9]:
  pts=rings[j]+[rings[j][0]];tube('Airframe hoop '+str(j),pts,.028 if j>=6 else .018,'edge')
# Nose glazing longerons define the rounded canopy, rather than a flat box.
for i in [0,4,8,12,20,24,28]:tube('Canopy longitudinal frame',[rings[j][i] for j in range(6,11)],.027,'edge')
for sign in [-1,1]:
 tube('Canopy sill',[(sign*1.25,.6,.55),(sign*1.2,1.35,.55),(sign*.99,2.05,.51),(sign*.79,2.65,.45),(sign*.4,3.15,.35)],.045,'dark')
 tube('Cabin door frame',[(sign*1.10,-2.3,.1),(sign*1.28,-2.3,1.6),(sign*1.28,-.4,1.65),(sign*1.20,-.4,.1)],.045,'edge')
 # Door slides rearward and has its own panel/handle.
 box('Armored sliding door',(sign*1.05,-2.7,.9),(.06,1.0,1.45),'olive',.06,rot=(0,sign*.1,0))
 tube('Door grab bar',[(sign*1.12,-2.45,.7),(sign*1.14,-2.45,.92),(sign*1.12,-2.45,1.14)],.018,'steel')
 tube('Boarding step',[(sign*1.18,-1.9,-.05),(sign*1.65,-1.9,-.38),(sign*1.65,-.75,-.38),(sign*1.18,-.75,-.05)],.04,'steel')
 for y in [-2.2,-1.8,-1.4,-1,-.6]:
  cyl('Door frame rivet',(sign*1.25,y,.12),.012,.01,'steel',rot=(0,math.pi/2,0),verts=10)
# Deck and cabin equipment.
box('Cabin floor',(0,-.4,-.04),(2.32,4.3,.09),'dark',.03)
for x in [-.86,-.43,0,.43,.86]:box('Anti-slip deck runner',(x,-.3,.018),(.20,4.0,.018),'rubber',.006)
for y in [-2.1,-1.55,-1,-.45,.1,.65]:
 for x in [-1.05,1.05]:cyl('Cargo tiedown',(x,y,.025),.045,.012,'steel',verts=16)
for y in [-1.75,-.8,.15]:
 tube('Interior ceiling rib',[(1.1*math.sin(t),y,.82+1.04*math.cos(t)) for t in [-1.4,-1.1,-.7,0,.7,1.1,1.4]],.028,'steel')
 box('Cabin lamp housing',(0,y,1.86),(.34,.16,.07),'dark',.025);box('Cabin lamp diffuser',(0,y,1.815),(.28,.11,.014),'ivory',.01)
for sign in [-1,1]:
 tube('Protected cabin wiring',[(sign*.85,-2.1,1.54),(sign*.87,-1.2,1.7),(sign*.83,.5,1.7),(sign*.8,1.0,1.6)],.028,'rubber')
 cyl('Fire bottle',(sign*.96,-2.05,.66),.11,.59,'red');cyl('Fire bottle top',(sign*.96,-2.05,.98),.045,.08,'steel')
 for z in [.48,.78]:torus('Bottle clamp',(sign*.96,-2.05,z),.113,.012,'steel')

# Pilot/copilot seats have shaped cushions, harnesses and independent metal frames.
def seat(x,y,name,group='FCAirframe'):
 box(name+' seat pan',(x,y,.37),(.54,.57,.10),'edge',.045)
 box(name+' cushion',(x,y,.45),(.51,.53,.13),'leather',.055)
 box(name+' back shell',(x,y-.25,.83),(.56,.12,.85),'edge',.06,rot=(-.10,0,0))
 box(name+' back cushion',(x,y-.17,.85),(.49,.14,.73),'leather',.055,rot=(-.10,0,0))
 for z in [.58,.72,.86,1.0,1.14]:tube(name+' upholstery seam',[(x-.22,y-.09-(z-.58)*.1,z),(x+.22,y-.09-(z-.58)*.1,z)],.004,'dark')
 for dx in [-.19,.19]:
  tube(name+' harness',[(x+dx,y+.16,.52),(x+dx*.8,y-.07,.8),(x+dx,y-.11,1.18)],.018,'rubber')
  tube(name+' seat frame',[(x+dx,y+.17,.04),(x+dx,y+.17,.36),(x+dx,y-.20,.36),(x+dx,y-.28,1.24)],.022,'steel')
 cyl(name+' harness buckle',(x,y+.13,.53),.047,.018,'brass',rot=(math.pi/2,0,0))
 box(name+' head support',(x,y-.27,1.31),(.36,.16,.19),'leather',.055)
for x in [-.58,.58]:seat(x,1.08,'Pilot' if x>0 else 'Copilot')
for y in [-1.62,-.72]:
 for x in [-.76,.76]:seat(x,y,'Troop seat')

# Contoured three-section flight panel with actual bezels, glass, needles and legends.
a=math.radians(65);rot=(a,0,0);U=Vector((1,0,0));V=Vector((0,math.cos(a),math.sin(a)));F=Vector((0,-math.sin(a),math.cos(a)));P=Vector((0,1.95,.88))
def pp(x,y,z=0):return P+U*x+V*y+F*z
for x in [-.65,0,.65]:box('Removable instrument panel',pp(x,0),( .62,.56,.095),'dark',.04,rot)
tube('Padded curved glare shield',[pp(x,.31,.06) for x in [-.98,-.8,-.5,0,.5,.8,.98]],.045,'rubber')
def dial(x,y,r,title,reading=.45):
 cyl(title+' bezel',pp(x,y,.065),r,.035,'steel',rot,verts=32);cyl(title+' face',pp(x,y,.087),r*.89,.008,'dark',rot,verts=32)
 for i in range(15):
  th=math.radians(-135+270*i/14);v=pp(x+math.sin(th)*r*.72,y+math.cos(th)*r*.72,.094)
  o=box(title+' tick',v,(.003,.012 if i%2 else .020,.0015),'ivory',0,rot);o.rotation_euler.rotate_axis('Z',-th)
 th=math.radians(-135+270*reading);tube(title+' needle',[pp(x,y,.099),pp(x+math.sin(th)*r*.63,y+math.cos(th)*r*.63,.099)],.003,'lume');cyl(title+' hub',pp(x,y,.103),.009,.008,'steel',rot,verts=12)
 label(title+' lettering',title,pp(x,y-r*.32,.096),r*.16,rot)
for sign in [-1,1]:
 for x,y,r,title,val in [(sign*.66,.11,.105,'ROTOR RPM',.65),(sign*.88,.11,.085,'FUEL',.7),(sign*.45,.11,.085,'OIL',.5),(sign*.66,-.13,.09,'ALTITUDE',.3),(sign*.87,-.13,.072,'TEMP',.5),(sign*.45,-.13,.072,'VOLTS',.6)]:dial(x,y,r,title,val)
dial(0,.07,.16,'ATTITUDE',.51)
for x in [-.11,0,.11]:
 box('Annunciator surround',pp(x,-.20,.07),(.085,.055,.022),'steel',.008,rot);box('Annunciator lens',pp(x,-.20,.087),(.071,.039,.008),'lume' if x==0 else 'amber',.005,rot)
for x in [-.94,-.35,.35,.94]:
 for y in [-.24,.24]:cyl('Quarter-turn panel fastener',pp(x,y,.06),.009,.006,'brass',rot,verts=12)
label('Flight panel designation','FC-27  /  FLIGHT SYSTEMS',pp(0,.27,.07),.024,rot)
# Center console with radio, guarded switches and reachable cyclics.
box('Central systems pedestal',(0,1.39,.48),(.35,.77,.64),'olive',.065)
for y in [1.13,1.36,1.59]:
 box('Radio module',(0,y,.818),(.30,.19,.028),'dark',.012)
 label('Radio legend','VHF  /  COM' if y==1.13 else 'PWR  /  BUS',(0,y-.02,.84),.020,(0,0,0))
 for x in [-.115,.115]:cyl('Knurled radio knob',(x,y,.865),.025,.040,'rubber',verts=16)
for sign in [-1,1]:
 group='FCCyclicRight' if sign>0 else 'FCCyclicLeft';x=sign*.58
 for z,r in [(.26,.085),(.30,.075),(.34,.065)]:torus('Cyclic accordion boot',(x,1.60,z),r,.016,'rubber',group=group)
 tube('Cyclic bent shaft',[(x,1.60,.26),(x,1.62,.48),(x,1.52,.70)],.021,'steel',group)
 box('Cyclic sculpted grip',(x,1.52,.77),(.063,.082,.18),'rubber',.025,rot=(-.22,0,0),group=group)
 cyl('Cyclic thumb button',(x,1.50,.87),.014,.009,'red',group=group,verts=16)
 for z in [.71,.75,.79]:tube('Cyclic finger ridge',[(x-.031,1.475,z),(x+.031,1.475,z)],.007,'dark',group)
 tube('Collective lever',[(x+sign*.27,.91,.40),(x+sign*.27,1.35,.62)],.024,'steel')
 box('Collective handgrip',(x+sign*.27,1.35,.63),(.07,.18,.07),'rubber',.026)
 for dx in [-.12,.12]:
  tube('Rudder pedal arm',[(x+dx,1.85,.05),(x+dx,2.02,.19)],.019,'steel');box('Rudder pedal',(x+dx,2.04,.20),(.18,.11,.035),'steel',.012,rot=(.35,0,0))
  for xx in [-.06,0,.06]:box('Pedal anti-slip',(x+dx+xx,2.04,.221),(.018,.09,.009),'rubber',.004)
# Overhead switchbox and plumbed canopy frame.
box('Overhead systems box',(0,1.25,1.88),(.42,.62,.12),'dark',.045)
for y in [.99,1.15,1.31,1.47]:
 for x in [-.13,0,.13]:
  cyl('Overhead toggle collar',(x,y,1.81),.014,.01,'steel',verts=12);tube('Overhead toggle',[(x,y,1.805),(x,y+.015,1.765)],.006,'steel')

# Main wing, twin lift nacelles and independently named rotor pivots.
for sign in [-1,1]:
 vs=[(sign*x,y,z) for x,y,z in [(1.0,-1.1,1.55),(4.05,-1.05,1.86),(4.05,-.25,1.86),(1.0,.28,1.55),(1.0,-1.1,1.76),(4.05,-1.05,2.02),(4.05,-.25,2.02),(1.0,.28,1.76)]]
 mesh('Swept lift wing',vs,[(0,1,2,3),(4,7,6,5),(0,4,5,1),(3,2,6,7),(1,5,6,2),(0,3,7,4)],'olive')
 tube('Wing hydraulic line',[(sign*1.1,-.95,1.82),(sign*2.7,-.84,2.04),(sign*3.75,-.8,2.10)],.022,'steel')
 cyl('Lift engine nacelle',(sign*4,-.65,1.94),.46,1.65,'olive',rot=(math.pi/2,0,0),verts=40)
 for y in [-1.49,.18]:
  torus('Nacelle rolled lip',(sign*4,y,1.94),.44,.038,'steel',rot=(math.pi/2,0,0));cyl('Nacelle intake shadow',(sign*4,y,1.94),.40,.025,'dark',rot=(math.pi/2,0,0))
 for dx in [-.3,-.2,-.1,0,.1,.2,.3]:tube('Intake grille',[(sign*4+dx,.20,1.65),(sign*4+dx,.20,2.23)],.012,'steel')
 for y in [-1.10,-.87,-.64,-.41]:box('Cooling louvre',(sign*4+.43,y,1.96),(.035,.14,.27),'dark',.008)
 cyl('Rotor transmission',(sign*4,-.65,2.31),.22,.40,'dark');torus('Transmission collar',(sign*4,-.65,2.42),.23,.025,'steel')
 group='FCRotorRight' if sign>0 else 'FCRotorLeft';pivot=pivots[group]
 cyl('Rotor hub',pivot,.20,.22,'steel',group=group)
 for k in range(3):
  angle=k*2*math.pi/3+(0 if sign>0 else .4);r=Vector((math.cos(angle),math.sin(angle),0));t=Vector((-math.sin(angle),math.cos(angle),0));c=Vector(pivot)
  pts=[c+r*.16-t*.09,c+r*2.6-t*.075+Vector((0,0,-.04)),c+r*2.68+t*.14+Vector((0,0,.015)),c+r*.25+t*.19]
  mesh('Composite rotor blade',[list(p) for p in pts],[(0,1,2,3)],'dark',group,solid=.025)
  pts2=[c+r*2.35-t*.076+Vector((0,0,.015)),c+r*2.51-t*.075+Vector((0,0,.015)),c+r*2.57+t*.14+Vector((0,0,.03)),c+r*2.40+t*.145+Vector((0,0,.03))]
  mesh('Rotor tip warning stripe',[list(p) for p in pts2],[(0,1,2,3)],'ivory',group)
  tube('Rotor pitch link',[c+r*.10+Vector((0,0,-.10)),c+r*.42+Vector((0,0,.02))],.018,'brass',group)
# Tail surfaces and trim tabs.
mesh('Vertical tail fin',[(-.05,-4.4,.9),(-.05,-5.4,.85),(-.05,-5.1,2.45),(-.05,-4.65,2.6)],[(0,1,2,3)],'olive',solid=.11)
for sign in [-1,1]:
 mesh('Tail stabilizer',[(0,-4.3,.8),(sign*1.45,-4.7,.9),(sign*1.5,-5.1,.9),(0,-5.2,.8)],[(0,1,2,3)],'olive',solid=.09)
 tube('Tail elevator hinge',[(0,-4.95,.86),(sign*1.45,-4.96,.96)],.018,'steel')
# Working-rig geometry: separate gear assemblies, struts and wheels.
for name,x,y in [('Nose',0,2.2),('Left',-1.0,-1.35),('Right',1.0,-1.35)]:
 group='FCGear'+name;px=x*1.25
 tube(name+' gear main strut',[(x,y,-.25),(px,y+.12,-.87)],.055,'steel',group)
 tube(name+' exposed oleo',[(px,y+.12,-.64),(px,y+.12,-1.03)],.034,'steel',group)
 tube(name+' gear brace',[(x*.7,y-.45,-.15),(px,y+.12,-.82)],.027,'edge',group)
 for side in [-1,1]:
  xx=px+side*.12;torus(name+' tire',(xx,y+.12,-1.03),.22,.085,'rubber',rot=(0,math.pi/2,0),group=group);cyl(name+' wheel hub',(xx,y+.12,-1.03),.145,.16,'steel',rot=(0,math.pi/2,0),group=group)
# Original markings and recognizable service hardware.
label('Nose stencil','FC-27',(0,3.455,.43),.12,(math.pi/2,0,math.pi))
for sign in [-1,1]:
 for y in [-3.25,-2.9]:
  cyl('Exhaust shroud',(sign*.65,y,1.12),.16,.35,'steel',rot=(0,math.pi/2,0));cyl('Exhaust soot',(sign*.84,y,1.12),.12,.02,'dark',rot=(0,math.pi/2,0))
 cyl('Landing lamp case',(sign*.38,2.95,.06),.13,.16,'dark',rot=(math.pi/2,0,0));cyl('Landing lamp glass',(sign*.38,3.04,.06),.105,.015,'ivory',rot=(math.pi/2,0,0))
 tube('Upper antenna',[(sign*.5,-2.4,1.75),(sign*.52,-2.55,2.45)],.012,'steel')
 for j in [2,5,6]:
  ring=rings[j]
  for i in range(0,N,2):
   pos=Vector(ring[i]);normal=Vector((pos.x,0,pos.z-sections[j][2])).normalized();o=cyl('Hull flush rivet',pos+normal*.023,.010,.009,'steel',verts=8);o.rotation_euler=normal.to_track_quat('Z','Y').to_euler()
for sign in [-1,1]:
 label('Original side designation','FC-27',(sign*1.305,.15,.96),.20,(math.pi/2,0,sign*math.pi/2))
 label('Side service stencil','RESCUE / UTILITY',(sign*1.305,.15,.76),.045,(math.pi/2,0,sign*math.pi/2))
 box('Wingtip navigation housing',(sign*4.46,-.45,2.10),(.06,.17,.10),'dark',.025)
 box('Wingtip navigation lens',(sign*4.495,-.45,2.10),(.018,.12,.07),'red' if sign<0 else 'lume',.012)
# Aircraft rig roots are retained as named animation pivots in Blender and NIF.
for name,point in pivots.items():
 empty=bpy.data.objects.new(name,None);empty.location=point;bpy.context.collection.objects.link(empty)
 bpy.context.view_layer.update()
 for o in parts:
  if o['group']==name:world=o.matrix_world.copy();o.parent=empty;o.matrix_world=world
bpy.context.view_layer.update();meshes=[];tri_count=0
for o in parts:
 me=o.data;me.calc_loop_triangles();vs=[];ns=[];ts=[]
 for tri in me.loop_triangles:
  xyz=[o.matrix_world@me.vertices[i].co for i in tri.vertices]
  if (xyz[1]-xyz[0]).cross(xyz[2]-xyz[0]).length<1e-10:continue
  n0=len(vs);vs.extend([list(v) for v in xyz]);ns.extend([list((o.matrix_world.to_3x3()@me.corner_normals[j].vector).normalized()) for j in tri.loops]);ts.append([n0,n0+1,n0+2])
 tri_count+=len(ts);meshes.append({'name':o.name,'group':o['group'],'material':me.materials[0].name,'vertices':vs,'normals':ns,'triangles':ts})
(OUT/'geometry.json').write_text(json.dumps({'materials':COLORS,'pivots':pivots,'meshes':meshes}),encoding='utf8')
contract={'units':'metres','airframe_origin':[0,0,405],'units_per_metre':69.99,'pilot_eye':[.58,1.08,1.30],'pilot_anchor':[.58*69.99,1.08*69.99+128,405+(1.30-1.1)*69.99],'passengers':[[-.76,-1.62,.45],[.76,-1.62,.45],[-.76,-.72,.45],[.76,-.72,.45]],'cyclic_right':[.58,1.52,.77],'original_assets':True,'source':'vehicle-vr/build_original_aircraft.py','triangles':tri_count,'pivots':pivots}
(OUT/'contract.json').write_text(json.dumps(contract,indent=2),encoding='utf8')
# Studio views are actual source geometry, never substituted for game acceptance.
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True;scene.render.resolution_x=1600;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
world=bpy.data.worlds.new('Neutral hangar');scene.world=world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.12,.15,.16,1);world.node_tree.nodes['Background'].inputs[1].default_value=.55
for loc,energy,size,target in [((2,6,9),2800,8,(0,0,.5)),((-6,1,5),2400,7,(0,0,.7)),((3,-6,5),3000,6,(0,-1,1)),((0,1.2,1.8),50,1,(0,1.4,.7))]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=energy;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-1.34));floor=bpy.context.object;floor.name='Studio floor NOT EXPORTED';m=bpy.data.materials.new('Studio floor');m.diffuse_color=(.075,.10,.10,1);floor.data.materials.append(m)
bpy.ops.object.camera_add();cam=bpy.context.object;cam.data.clip_start=.02;scene.camera=cam
for name,loc,target,lens in [('exterior',(12,15,8),(0,-.4,.5),45),('pilot',(.58,1.08,1.30),(.58,3.4,.83),23),('boarding',(5,-3,2),(0,-.75,.65),36),('rear',(-10,-12,6),(0,-1,.7),45)]:
 cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=lens;scene.render.filepath=str(OUT/(name+'.png'));bpy.ops.render.render(write_still=True)
bpy.context.preferences.filepaths.save_version=0;bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'FC-27-original-aircraft.blend'))
print('ORIGINAL model complete:',len(parts),'objects;',tri_count,'triangles; no retail asset inputs')
