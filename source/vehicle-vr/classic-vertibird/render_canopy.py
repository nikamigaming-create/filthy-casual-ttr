"""Render the private classic canopy proposal; not game acceptance evidence."""
import bpy,json,sys,bmesh
from pathlib import Path
from mathutils import Vector
args=sys.argv[sys.argv.index('--')+1:];out=Path(args[0]);g=json.loads((out/'canopy-geometry.json').read_text(encoding='utf8'))
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
for m in g['meshes']:
 me=bpy.data.meshes.new(m['name']);me.from_pydata([[x/70 for x in v] for v in m['vertices']],[],m['triangles']);me.update()
 ob=bpy.data.objects.new(m['name'],me);bpy.context.collection.objects.link(ob);ob['source_group']=m['group']
 if m['uvs']:
  uv=me.uv_layers.new()
  for i,loop in enumerate(me.loops):u,v=m['uvs'][loop.vertex_index];uv.data[i].uv=(u,1-v)
 mat=bpy.data.materials.new(m['name']+' source material');mat.use_nodes=True;nodes=mat.node_tree.nodes;links=mat.node_tree.links;bs=nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.68
 if m['diffuse']:
  tex=nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(g['textures'][m['diffuse']],check_existing=True);links.new(tex.outputs['Color'],bs.inputs['Base Color'])
 if m['normal']:
  tex=nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(g['textures'][m['normal']],check_existing=True);tex.image.colorspace_settings.name='Non-Color'
  sep=nodes.new('ShaderNodeSeparateColor');comb=nodes.new('ShaderNodeCombineColor');inv=nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
  links.new(tex.outputs['Color'],sep.inputs[0]);links.new(sep.outputs[0],comb.inputs[0]);links.new(sep.outputs[1],inv.inputs[1]);links.new(inv.outputs[0],comb.inputs[1]);links.new(sep.outputs[2],comb.inputs[2]);nm=nodes.new('ShaderNodeNormalMap');links.new(comb.outputs[0],nm.inputs['Color']);links.new(nm.outputs[0],bs.inputs['Normal'])
 if m.get('glass'):
  bs.inputs['Base Color'].default_value=(.30,.43,.43,1);bs.inputs['Roughness'].default_value=.07;bs.inputs['Transmission Weight'].default_value=1;bs.inputs['IOR'].default_value=1.42
  mix=nodes.new('ShaderNodeMixShader');mix.inputs[0].default_value=.65;transparent=nodes.new('ShaderNodeBsdfTransparent');links.new(transparent.outputs[0],mix.inputs[1]);links.new(bs.outputs[0],mix.inputs[2]);links.new(mix.outputs[0],nodes.get('Material Output').inputs['Surface'])
 me.materials.append(mat)
 bm=bmesh.new();bm.from_mesh(me);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001);bm.to_mesh(me);bm.free();me.update()
 if m.get('glass') and len(me.polygons):
  bm=bmesh.new();bm.from_mesh(me)
  rubber=bpy.data.materials.get('Canopy rubber seal') or bpy.data.materials.new('Canopy rubber seal');rubber.use_nodes=True;rubber.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.025,.03,.028,1);rubber.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.8
  for edge in bm.edges:
   if not edge.is_boundary:continue
   curve=bpy.data.curves.new('Recessed canopy gasket','CURVE');curve.dimensions='3D';curve.bevel_depth=.018;curve.bevel_resolution=2;spline=curve.splines.new('POLY');spline.points.add(1)
   for dest,vert in zip(spline.points,edge.verts):dest.co=(*vert.co,1)
   seal=bpy.data.objects.new('Canopy gasket',curve);bpy.context.collection.objects.link(seal);seal['source_group']='exterior';curve.materials.append(rubber)
  bm.free()
 for p in me.polygons:p.use_smooth=True
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1440;scene.render.resolution_y=1080;scene.render.resolution_percentage=100
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.38,.43,.50,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.5
for pos,power,size in [((0,-8,10),2200,8),((-8,3,8),1800,7),((0,-1,.5),35,2)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,2))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();cam=bpy.context.object;scene.camera=cam
for name,pos,target,lens,groups in [('canopy-exterior',(14,-19,11),(0,-.5,3),46,{'exterior'}),('canopy-pilot',(-.6,-4.07,3.85),(-.6,-7,3.8),22,{'exterior'})]:
 for ob in bpy.data.objects:
  if 'source_group' in ob:ob.hide_render=ob['source_group'] not in groups or ob.name=='VertibirdHull01:2'
 cam.location=pos;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=lens;scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
bpy.context.preferences.filepaths.save_version=0;bpy.ops.wm.save_as_mainfile(filepath=str(out/'classic-canopy-proposal.blend'))
