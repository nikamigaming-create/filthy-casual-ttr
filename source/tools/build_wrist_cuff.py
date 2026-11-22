"""Original curved cuff concept and physical plungers; headless Blender only.
Centimetres; neutral forearm proxy is preview-only and never exported.
"""
import bpy,bmesh,math,json,random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'wrist-controls'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
COLORS={'olive':(.072,.091,.046),'brass':(.28,.23,.13),'ivory':(.69,.67,.51),
 'dark':(.022,.027,.023),'amber':(.74,.29,.028),'steel':(.14,.16,.15),'proxy':(.23,.26,.27)}
mats={};parts=[];presses=[]
for name,rgb in COLORS.items():
 m=bpy.data.materials.new(name);m.use_nodes=True;p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
 p.inputs['Base Color'].default_value=(*rgb,1);p.inputs['Metallic'].default_value=.7 if name in ('olive','brass','steel') else .1
 p.inputs['Roughness'].default_value=.48 if name!='proxy' else .78;m.diffuse_color=(*rgb,1);mats[name]=m
def finish(o,name,mat,group='FC_Plate',moving=False,bevel=0):
 o.name=name;o.data.materials.append(mats[mat]);o['group']=group;o['moving']=moving
 if bevel:
  bpy.context.view_layer.objects.active=o;mod=o.modifiers.new('Machined fillet','BEVEL');mod.width=bevel;mod.segments=2
  bpy.ops.object.modifier_apply(modifier=mod.name)
 parts.append(o)
 if moving:presses.append(o)
 return o
def box(name,p,d,mat,group='FC_Plate',moving=False,bevel=.02):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.dimensions=d
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 return finish(o,name,mat,group,moving,bevel)
def cylinder(name,x,y,z,r,h,mat,group='FC_Plate',moving=False,bevel=.015):
 bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=r,depth=h,location=(x,y,z))
 return finish(bpy.context.object,name,mat,group,moving,bevel)
def mesh(name,v,f,mat,group='FC_Plate',moving=False):
 m=bpy.data.meshes.new(name);m.from_pydata(v,[],f);m.update()
 bm=bmesh.new();bm.from_mesh(m);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(m);bm.free()
 o=bpy.data.objects.new(name,m);bpy.context.collection.objects.link(o)
 return finish(o,name,mat,group,moving)
def ring(name,x,y,z,ro,ri,h,mat,group='FC_Plate',moving=False,n=48):
 v=[]
 for zz in (z-h/2,z+h/2):
  for r in (ro,ri):
   v.extend([(x+r*math.cos(i*math.tau/n),y+r*math.sin(i*math.tau/n),zz) for i in range(n)])
 f=[]
 for i in range(n):
  j=(i+1)%n;f.extend([(i,j,n+j,n+i),(2*n+i,3*n+i,3*n+j,2*n+j),(i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)])
 return mesh(name,v,f,mat,group,moving)
def difference(o,cutter,remove=True):
 bpy.context.view_layer.objects.active=o;mod=o.modifiers.new('Recess','BOOLEAN');mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=cutter
 bpy.ops.object.modifier_apply(modifier=mod.name)
 if remove:
  if cutter in parts:parts.remove(cutter)
  if cutter in presses:presses.remove(cutter)
  bpy.data.objects.remove(cutter,do_unlink=True)
def polygon(name,xy,z,h,mat,group,moving=True):
 n=len(xy);v=[(x,y,z-h/2) for x,y in xy]+[(x,y,z+h/2) for x,y in xy]
 f=[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
 return mesh(name,v,f,mat,group,moving)

# Curved shell with a rounded perimeter, not a flat slab. Top camber wraps forearm.
outline=[]
for cx,cy,start in [(2.8,1.8,0),(-2.8,1.8,90),(-2.8,-1.8,180),(2.8,-1.8,270)]:
 for i in range(9):
  a=math.radians(start+i*90/8);outline.append((cx+.7*math.cos(a),cy+.7*math.sin(a)))
def surf(x,y):return .645-.055*x*x-.012*y*y
v=[];n=len(outline);levels=6
for side in (0,1):
 for k in range(levels):
  s=1-k/(levels-.4)
  v.extend([(x*s,y*s,surf(x*s,y*s)-(.27 if side==0 else 0)) for x,y in outline])
f=[]
for side in (0,1):
 off=side*levels*n
 for k in range(levels-1):
  for i in range(n):
   j=(i+1)%n;f.append((off+k*n+i,off+k*n+j,off+(k+1)*n+j,off+(k+1)*n+i))
 f.append(tuple(off+(levels-1)*n+i for i in range(n)))
for i in range(n):j=(i+1)%n;f.append((i,j,levels*n+j,levels*n+i))
body=mesh('Cambered cast housing',v,f,'olive')
# Integrated cuff follows an elliptical wrist, broad at housing and slim underneath.
v=[];f=[];n=64
for yy in (-1.55,1.55):
 for inner in (False,True):
  for i in range(n):
   a=i*math.tau/n;rx=3.08 if inner else 3.24;rz=2.60 if inner else 2.76
   v.append((rx*math.cos(a),yy,-2.6+rz*math.sin(a)))
for i in range(n):
 j=(i+1)%n;f.extend([(i,j,n+j,n+i),(2*n+i,3*n+i,3*n+j,2*n+j),(i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)])
cuff=mesh('Thin fitted metal cuff',v,f,'steel')
# Rubber inner liner, small screw-fastened hinge and clasp are physical parts.
for x in (-3.18,3.18):
 box('Cuff hinge leaf',(x,0,-1.18),(.22,1.65,.65),'brass',bevel=.08)
 for y in (-.55,.55):
  screw=cylinder('Hinge pin',x,y,-.80,.09,.10,'steel');screw.rotation_euler.y=math.pi/2
box('Underside clasp',(0,0,-5.35),(1.45,1.18,.13),'dark',bevel=.10)
box('Clasp catch',(0,0,-5.44),(.68,.72,.09),'brass',bevel=.05)
CENTERS={'FC_Weapons':(-1.65,1,.48),'FC_Consumables':(1.65,1,.48),
 'FC_Reload_Auto':(-1.65,-1.30,.46),'FC_Reload_Manual':(-1.65,-1.30,.46),'FC_Settings':(1.65,-1.30,.46)}
buttons={}
for group,(x,y,z) in CENTERS.items():
 big=group in ('FC_Weapons','FC_Consumables');r=1.2 if big else .9
 if group!='FC_Reload_Manual':
  cut=cylinder('Temporary well cutter',x,y,.25,r-.025,2,'dark',bevel=0);difference(body,cut)
  cut=cylinder('Temporary cuff cutter',x,y,.1,r-.04,2,'dark',bevel=0);difference(cuff,cut)
 ring(group+' recessed collar',x,y,z-.13,r,r-.16,.36,'steel',group)
 ring(group+' rolled rim',x,y,z+.023,r-.018,r-.17,.07,'brass',group)
 cylinder(group+' well floor',x,y,z-.285,r-.15,.055,'dark',group)
 cap=cylinder(group+' enamel plunger',x,y,z-.105,r-.20,.21,'dark' if not big else 'olive',group,True,.025)
 buttons[group]=cap
 # Two small actual fasteners around a slim fixed collar.
 for a in (0,math.pi):
  xx=x+(r-.075)*math.cos(a);yy=y+(r-.075)*math.sin(a)
  cylinder(group+' collar screw',xx,yy,z+.062,.048,.025,'steel',group,False,0)

glyphs=[]
def glyphpoly(name,p,z,group,mat='ivory'):
 ob=polygon(name,p,z+.015,.11,mat,group);glyphs.append(ob);return ob
def glyphstroke(name,points,z,group,mat='ivory',width=.11):
 for i,(a,b) in enumerate(zip(points,points[1:])):
  dx=b[0]-a[0];dy=b[1]-a[1]
  ob=box(name+str(i),((a[0]+b[0])/2,(a[1]+b[1])/2,z+.015+i*.002),
   (math.hypot(dx,dy)+width,width,.11),mat,group,True,0);ob.rotation_euler.z=math.atan2(dy,dx);glyphs.append(ob)
x,y,z=CENTERS['FC_Weapons']
glyphpoly('Inlaid pistol',[(x-.62,y+.25),(x+.60,y+.25),(x+.60,y+.04),(x+.05,y+.04),
 (x-.04,y-.10),(x+.06,y-.45),(x-.26,y-.45),(x-.43,y+.04),(x-.62,y+.04)],z,'FC_Weapons')
x,y,z=CENTERS['FC_Consumables']
# First aid plus is intentionally simple and readable at actual wrist size.
glyphpoly('Inset supply cross',[(x-.17,y+.55),(x+.17,y+.55),(x+.17,y+.17),(x+.55,y+.17),
 (x+.55,y-.17),(x+.17,y-.17),(x+.17,y-.55),(x-.17,y-.55),(x-.17,y-.17),(x-.55,y-.17),(x-.55,y+.17),(x-.17,y+.17)],z,'FC_Consumables')
x,y,z=CENTERS['FC_Reload_Auto']
glyphstroke('Automatic A',[(x-.31,y-.34),(x,y+.34),(x+.31,y-.34)],z,'FC_Reload_Auto','amber')
glyphstroke('A crossbar',[(x-.19,y-.10),(x+.19,y-.10)],z+.004,'FC_Reload_Auto','amber')
glyphstroke('Manual M',[(x-.31,y-.34),(x-.31,y+.34),(x,y-.04),(x+.31,y+.34),(x+.31,y-.34)],z,'FC_Reload_Manual')
x,y,z=CENTERS['FC_Settings'];xy=[]
for i in range(48):
 a=i*math.tau/48;r=.51 if i%6 in (1,2,3) else .42;xy.append((x+r*math.cos(a),y+r*math.sin(a)))
gear=glyphpoly('Inset gear',xy,z,'FC_Settings');cut=cylinder('Gear center',x,y,z,.21,.5,'dark',bevel=0);difference(gear,cut)
# Cut actual shallow icon recesses and sink the original ivory/amber inlays.
for ob in glyphs:
 bpy.context.view_layer.update();difference(buttons[ob['group']],ob,False)
 ob.location.z-=.061
random.seed(70)
for i in range(12):
 x=random.uniform(-2.9,2.9);y=2.31 if i%2 else -2.31
 box('Small enamel edge wear',(x,y,surf(x,y)+.003),(.055,.015,.006),'brass',bevel=0)
# Four service screws follow the housing camber rather than floating on a slab.
for x in (-3.1,3.1):
 for y in (-1.95,1.95):
  z=surf(x,y)+.018;cylinder('Flush housing fastener',x,y,z,.105,.035,'steel',bevel=.006)
  box('Fastener slot',(x,y,z+.018),(.12,.018,.005),'dark',bevel=0)

# Correct all winding, then emit exactly the model geometry (no proxy).
for ob in parts:
 bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free()
for ob in (body,cuff):
 for p in ob.data.polygons:p.use_smooth=True
 # Analytical surface normals keep boolean well walls hard and the cambered
 # outer skin smooth; averaging both produces artificial dents around holes.
 normals=[]
 for p in ob.data.polygons:
  for li in p.loop_indices:
   co=ob.data.vertices[ob.data.loops[li].vertex_index].co
   if ob==body and p.normal.z>.5:
    normal=Vector((.11*co.x,.024*co.y,1)).normalized()
   elif ob==body and p.normal.z<-.5:
    normal=-Vector((.11*co.x,.024*co.y,1)).normalized()
   elif ob==cuff and abs(p.normal.y)<.6:
    normal=Vector((co.x/(3.24**2),0,(co.z+2.6)/(2.76**2))).normalized()
    if normal.dot(p.normal)<0:normal=-normal
   else:normal=p.normal.copy()
   normals.append(tuple(normal))
 ob.data.normals_split_custom_set(normals)
bpy.context.view_layer.update();meshes=[]
for ob in parts:
 me=ob.data;me.calc_loop_triangles();vs=[];ns=[];ts=[]
 for tri in me.loop_triangles:
  xyz=[ob.matrix_world@me.vertices[i].co for i in tri.vertices];normal=(xyz[1]-xyz[0]).cross(xyz[2]-xyz[0])
  if normal.length<1e-9:continue
  normal.normalize();base=len(vs);vs.extend([list(v) for v in xyz])
  ns.extend([list((ob.matrix_world.to_3x3()@me.corner_normals[li].vector).normalized()) for li in tri.loops])
  ts.append([base,base+1,base+2])
 meshes.append({'name':ob.name,'group':ob['group'],'moving':bool(ob['moving']),
  'material':me.materials[0].name,'vertices_cm':vs,'normals':ns,'triangles':ts})
(OUT/'geometry.json').write_text(json.dumps({'materials':{k:v for k,v in COLORS.items() if k!='proxy'},'meshes':meshes}))
contract={'root':'FC_WristControls','units_per_metre':69.99125,'units_per_cm':.6999125,
 'axes':{'x':'across wrist','y':'toward elbow','z':'outward from dorsal wrist'},'contact':'flat top contact planes; all press translations along root -Z',
 'travel_cm':.16,'activation_depth_cm':.12,'buttons':[]}
for name,c in CENTERS.items():
 contract['buttons'].append({'node':name,'press_node':name+'_Press','center_cm':c,'center_game_units':[v*.6999125 for v in c],
  'contact_surface_z_cm':c[2],'contact_normal':[0,0,1],'press_translation_rest':[0,0,0],
  'radius_cm':1.2 if name in ('FC_Weapons','FC_Consumables') else .9})
(OUT/'attachment.json').write_text(json.dumps(contract,indent=2))
for ob in parts:ob.hide_render=ob['group']=='FC_Reload_Manual';ob.hide_set(ob.hide_render)

# Original neutral anatomical proxy, tapered elliptical forearm with wrist/hand transition.
v=[];f=[];n=64
for yy,rx,rz in [(-8,3.20,2.25),(-4,2.9,2.45),(0,3.02,2.59),(4,3.35,2.8),(8,3.75,3.1)]:
 v.extend([(rx*math.cos(i*math.tau/n),yy,-2.6+rz*math.sin(i*math.tau/n)) for i in range(n)])
for j in range(4):
 for i in range(n):k=(i+1)%n;f.append((j*n+i,j*n+k,(j+1)*n+k,(j+1)*n+i))
f.extend([tuple(reversed(range(n))),tuple(4*n+i for i in range(n))])
me=bpy.data.meshes.new('Neutral forearm proxy');me.from_pydata(v,[],f);me.update()
o=bpy.data.objects.new('PREVIEW ONLY - neutral anatomical forearm',me);bpy.context.collection.objects.link(o);me.materials.append(mats['proxy'])
for p in me.polygons:p.use_smooth=True
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
scene.render.engine='CYCLES';scene.cycles.samples=40;scene.render.resolution_x=1400;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world.color=(.15,.15,.15);scene.render.image_settings.file_format='PNG';scene.render.film_transparent=False
for pos,power,size in [((-5,-5,12),2200,7),((6,3,6),1100,6),((-4,6,2),700,5)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size
 o.rotation_euler=(Vector((0,0,-1))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();camera=bpy.context.object;camera.data.type='ORTHO';scene.camera=camera
def shot(name,pos,target,scale):
 camera.location=pos;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=scale
 scene.render.filepath=str(OUT/name);bpy.ops.render.render(write_still=True)
bpy.context.preferences.filepaths.save_version=0
shot('cuff-top.png',(0,0,19),(0,0,0),10)
shot('cuff-side.png',(13,-3,1),(0,0,-1.7),10)
shot('cuff-worn.png',(8,-10,11),(0,0,-.9),13)
for ob in presses:
 if ob['group']=='FC_Weapons':ob.location.z-=.16
shot('cuff-pressed.png',(5,-5,10),(-1.25,.55,.30),5.2)
for ob in presses:
 if ob['group']=='FC_Weapons':ob.location.z+=.16
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'filthy-casual-cuff.blend'))
print('Curved cuff source and four truthful proxy previews complete')
