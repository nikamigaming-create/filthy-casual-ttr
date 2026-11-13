"""Render source-faithful private classic Vertibird inspection views in Blender."""
import bpy,json,sys
from pathlib import Path
from mathutils import Vector
args=sys.argv[sys.argv.index('--')+1:];out=Path(args[0]);g=json.loads((out/'geometry.json').read_text(encoding='utf8'))
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
 me.materials.append(mat)
 for p in me.polygons:p.use_smooth=True
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1440;scene.render.resolution_y=1080;scene.render.resolution_percentage=100
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.38,.43,.50,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.5
for pos,power,size in [((0,-8,10),2200,8),((-8,3,8),1800,7),((0,-1,.5),35,2)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,2))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();cam=bpy.context.object;scene.camera=cam
for name,pos,target,lens,groups in [('exterior',(12,-15,9),(0,0,3),46,{'exterior'}),('pilot',(-.6,-1.35,-.22),(-.6,-3,-.55),24,{'interior','console'}),('interior-overview',(5,-5,3),(0,0,-.4),42,{'interior','console'})]:
 for ob in bpy.data.objects:
  if 'source_group' in ob:ob.hide_render=ob['source_group'] not in groups or ob.name=='VertibirdHull01:2'
 cam.location=pos;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=lens;scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
bpy.context.preferences.filepaths.save_version=0;bpy.ops.wm.save_as_mainfile(filepath=str(out/'classic-inspection.blend'))
