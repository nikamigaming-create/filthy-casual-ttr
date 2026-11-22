"""Author the flight lever geometry without reading game or dependency assets."""

import argparse
from math import cos, pi, sin, sqrt
from pathlib import Path
import time

time.clock = time.perf_counter
from pyffi.formats.nif import NifFormat as N


def node(name, position=(0, 0, 0)):
    part = N.NiNode()
    part.name, part.flags, part.scale = name.encode(), 14, 1
    part.rotation.set_identity()
    part.translation.x, part.translation.y, part.translation.z = position
    return part


def tube(parent, name, origin, end, radius, finish):
    origin, end = tuple(origin), tuple(end)
    axis = tuple(b - a for a, b in zip(origin, end))
    length = sqrt(sum(x * x for x in axis))
    _, y, z = (x / length for x in axis)
    shape = N.NiTriShape()
    shape.name, shape.flags, shape.scale = name.encode(), 14, 1
    shape.rotation.set_identity()
    data = N.NiTriShapeData()
    shape.data = data
    sides = 16
    data.num_vertices = sides * 2
    data.has_vertices = data.has_normals = True
    data.vertices.update_size()
    data.normals.update_size()
    data.num_uv_sets = 1
    data.uv_sets.update_size()
    for index in range(sides * 2):
        height, side = divmod(index, sides)
        angle = side * 2 * pi / sides
        normal = cos(angle), z * sin(angle), -y * sin(angle)
        p = origin if height == 0 else end
        v = data.vertices[index]
        v.x, v.y, v.z = (a + radius * n for a, n in zip(p, normal))
        n = data.normals[index]
        n.x, n.y, n.z = normal
        uv = data.uv_sets[0][index]
        uv.u, uv.v = side / sides, height
    triangles = []
    for i in range(sides):
        j = (i + 1) % sides
        triangles += [(i, j, i + sides), (j, j + sides, i + sides)]
    for i in range(1, sides - 1):
        triangles += [(0, i + 1, i), (sides, sides + i, sides + i + 1)]
    data.set_triangles(triangles)
    data.update_center_radius()
    shape.update_tangent_space()
    material = N.NiMaterialProperty()
    material.alpha, material.glossiness = 1, 25
    for color in (material.ambient_color, material.diffuse_color):
        color.r = color.g = color.b = 1
    shader = N.BSShaderPPLightingProperty()
    shader.shader_type = 1
    shader.shader_flags.sf_specular = 1
    shader.shader_flags.sf_z_buffer_test = 1
    shader.shader_flags_2.sf_2_z_buffer_write = 1
    textures = N.BSShaderTextureSet()
    textures.num_textures = 6
    textures.textures.update_size()
    textures.textures[0] = f'textures\\FilthyCasual\\lamp-{finish}.dds'.encode()
    textures.textures[1] = b'textures\\FilthyCasual\\lamp-flat_n.dds'
    shader.texture_set = textures
    shape.add_property(material)
    shape.add_property(shader)
    parent.add_child(shape)


def build(output):
    scene = N.Data(version=0x14020007, user_version=11, user_version_2=34)
    scene.header.endian_type = 1
    scene.header.export_info.creator = b'Filthy Casual TTR'
    root = node('FCFlightControlsModel')
    scene.roots = [root]
    right = node('FCFlightControl', (38, 160, 505))
    left = node('FCFlightCollective', (0, 133, 530))
    root.add_child(right)
    root.add_child(left)
    right.add_child(node('FCGimbalV2'))
    right.add_child(node('FCFlightGripR', (0, 0, 30)))
    left.add_child(node('FCFlightGripL', (0, 25, 8)))
    tube(root, 'Cyclic floor mount', (38, 160, 486), (38, 160, 505), 3.8, 'steel')
    tube(root, 'Collective floor mount', (0, 133, 486), (0, 133, 530), 3.8, 'steel')
    tube(right, 'Cyclic shaft', (0, 0, 0), (0, 0, 27), 1.2, 'steel')
    tube(right, 'Cyclic grip', (0, 0, 25), (0, 0, 36), 2.15, 'rubber')
    tube(left, 'Collective lever', (0, 0, 0), (0, 25, 8), 1.2, 'steel')
    tube(left, 'Collective grip', (0, 18, 6), (0, 29, 9.2), 2.15, 'rubber')
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('wb') as stream:
        scene.write(stream)
    check = N.Data()
    with output.open('rb') as stream:
        check.read(stream)
    names = {entry.name for entry in check.blocks if isinstance(entry, N.NiNode)}
    assert {b'FCFlightGripL', b'FCFlightGripR', b'FCGimbalV2'} <= names


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    build(parser.parse_args().output)
