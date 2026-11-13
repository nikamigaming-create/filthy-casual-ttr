"""Local-only cabin conversion. No dependency geometry is embedded here."""
import math
from pyffi.formats.nif import NifFormat as N

def clip_polygon(poly,axis,value,positive):
    if not poly:return []
    result=[]
    for a,b in zip(poly,poly[1:]+poly[:1]):
        da=(a[axis]-value)*(1 if positive else -1);db=(b[axis]-value)*(1 if positive else -1)
        ia=da>=-1e-6;ib=db>=-1e-6
        if ia:result.append(a)
        if ia!=ib:
            t=da/(da-db);result.append(tuple(x+(y-x)*t for x,y in zip(a,b)))
    return result

def subtract_box(poly,lo,hi):
    remaining=poly;outside=[]
    for axis in range(3):
        for value,positive in [(lo[axis],True),(hi[axis],False)]:
            cut=clip_polygon(remaining,axis,value,not positive)
            if len(cut)>=3:outside.append(cut)
            remaining=clip_polygon(remaining,axis,value,positive)
            if len(remaining)<3:return outside
    return outside

def open_windshields(root):
    old=next(b for b in root.tree() if isinstance(b,N.NiGeometry) and b.name==b'VertibirdHull01:0')
    source=old.data;uv_count=len(source.uv_sets);colors=bool(source.has_vertex_colors)
    attrs=[]
    for i,v in enumerate(source.vertices):
        a=list(v.as_list())+list(source.normals[i].as_list())
        for uv in source.uv_sets:a.extend([uv[i].u,uv[i].v])
        if colors:
            c=source.vertex_colors[i];a.extend([c.r,c.g,c.b,c.a])
        attrs.append(tuple(a))
    vertices=[];tris=[];lookup={}
    for indices in source.get_triangles():
        polys=[[attrs[i] for i in indices]]
        # Paired canopy apertures retain the center spine, outer rails, dashboard
        # and the cabin roof behind the pilot. Clip polygons at exact boundaries.
        for lo,hi in [((-98,-220,-52),(-3,-108,70)),((3,-220,-52),(98,-108,70))]:
            polys=[piece for poly in polys for piece in subtract_box(poly,lo,hi)]
        for poly in polys:
            ids=[]
            for v in poly:
                key=tuple(round(x,6) for x in v)
                if key not in lookup:lookup[key]=len(vertices);vertices.append(v)
                ids.append(lookup[key])
            for i in range(1,len(ids)-1):
                if len({ids[0],ids[i],ids[i+1]})==3:tris.append((ids[0],ids[i],ids[i+1]))
    assert len(vertices)<65535 and tris
    node=N.NiTriShape();node.name=old.name;node.flags=old.flags;node.set_transform(old.get_transform())
    for prop in old.properties:node.add_property(prop)
    data=N.NiTriShapeData();node.data=data;data.num_vertices=len(vertices);data.has_vertices=True;data.vertices.update_size();data.has_normals=True;data.normals.update_size()
    data.num_uv_sets=uv_count;data.has_uv=True;data.uv_sets.update_size();data.has_vertex_colors=colors;data.vertex_colors.update_size()
    for i,v in enumerate(vertices):
        data.vertices[i].x,data.vertices[i].y,data.vertices[i].z=v[:3]
        length=math.sqrt(sum(x*x for x in v[3:6])) or 1
        data.normals[i].x,data.normals[i].y,data.normals[i].z=[x/length for x in v[3:6]]
        for j in range(uv_count):data.uv_sets[j][i].u,data.uv_sets[j][i].v=v[6+j*2:8+j*2]
        if colors:data.vertex_colors[i].r,data.vertex_colors[i].g,data.vertex_colors[i].b,data.vertex_colors[i].a=v[6+uv_count*2:]
    data.num_triangles=len(tris);data.num_triangle_points=len(tris)*3;data.has_triangles=True;data.triangles.update_size()
    for t,ids in zip(data.triangles,tris):t.v_1,t.v_2,t.v_3=ids
    data.update_center_radius();node.update_tangent_space()
    parent=next(b for b in root.tree() if isinstance(b,N.NiNode) and old in list(b.children));parent.remove_child(old);parent.add_child(node)
    return {'vertices':len(vertices),'triangles':len(tris),'source_triangles':len(source.get_triangles())}
