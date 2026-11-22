"""Build the authored recorder optics and plugin. Fallout supplies the shell at runtime."""
import argparse
from pathlib import Path
import shutil
import struct
import time
from PIL import Image, ImageDraw

time.clock = time.perf_counter
from pyffi.formats.nif import NifFormat as N


def vector(target, xyz):
    target.x, target.y, target.z = xyz


def sub(tag, value):
    return tag.encode() + struct.pack('<H', len(value)) + value


def z(text):
    return text.encode('cp1252') + bytes(1)


def record(tag, form, body, flags=0):
    return struct.pack('<4sIIIIHH', tag.encode(), len(body), flags, form, 0, 15, 0) + body


def group(label, kind, body):
    if isinstance(label, int):
        label = struct.pack('<I', label)
    return struct.pack('<4sI4siHHHH', b'GRUP', len(body)+24, label, kind, 0, 0, 0, 0) + body


def screen(output):
    texture = output / 'textures/RobCoEyebot/screen.dds'
    texture.parent.mkdir(parents=True, exist_ok=True)
    image = Image.new('RGB', (128, 128), (12, 23, 15))
    draw = ImageDraw.Draw(image)
    draw.rectangle((5, 5, 122, 122), outline=(29, 67, 42), width=2)
    draw.text((64, 66), 'ROBCO', anchor='mm', fill=(70, 128, 85))
    image.save(texture)

    # An original curved CRT panel, fitted just forward of the stock glass.
    half_width, half_height, center = 7.8, 5.5, 2.35
    coords, indices = [], []
    columns, rows = 8, 6
    for row in range(rows + 1):
        v = row * 2 / rows - 1
        for column in range(columns + 1):
            u = column * 2 / columns - 1
            coords.append((half_width*u, 19.6 - 2.1*u*u - 1.1*v*v, center + half_height*v))
            if row and column:
                d = len(coords) - 1
                a = d - columns - 2
                indices.extend(((a, d, a+1), (a, d-1, d)))

    panel = N.NiTriShape()
    panel.name = b'RobCoEyebotScreen'
    panel.flags = 14
    panel.scale = 1
    panel.rotation.set_identity()
    data = N.NiTriShapeData()
    panel.data = data
    data.num_vertices = len(coords)
    data.has_vertices = data.has_normals = True
    data.vertices.update_size()
    data.normals.update_size()
    data.num_uv_sets = 1
    data.uv_sets.update_size()
    data.has_vertex_colors = True
    data.vertex_colors.update_size()
    for i, (x, y, z_pos) in enumerate(coords):
        vector(data.vertices[i], (x, y, z_pos))
        vector(data.normals[i], (0, 1, 0))
        data.uv_sets[0][i].u = .5 - x / (2 * half_width)
        data.uv_sets[0][i].v = .5 - (z_pos - center) / (2 * half_height)
        colour = data.vertex_colors[i]
        colour.r, colour.g, colour.b, colour.a = .27, 1., .42, 1.
    data.set_triangles(indices)
    data.update_center_radius()
    material = N.NiMaterialProperty()
    material.alpha = 1
    shader = N.BSShaderNoLightingProperty()
    shader.shader_type = 33
    shader.shader_flags.sf_z_buffer_test = 1
    shader.shader_flags_2.sf_2_z_buffer_write = 1
    shader.shader_flags_2.sf_2_no_fade = 1
    shader.shader_flags_2.sf_2_vertex_colors = 1
    shader.file_name = b'textures\\RobCoEyebot\\screen.dds'
    shader.falloff_start_opacity = shader.falloff_stop_opacity = 1
    panel.add_property(material)
    panel.add_property(shader)
    return panel


DEVICE_FUNCTIONS = {'Eyebot': ['AttachCamera', 'AttachOptics', 'BindRemote', 'CheckPath', 'Deploy', 'DetachCamera', 'Move', 'OnInventory', 'ProjectMovie', 'ProjectStill', 'Stow'], 'Voice': ['Equip', 'ItemCount', 'ItemName', 'LightOff', 'LightOn'], 'Wrist': ['BindClick', 'Click', 'ReadSticks', 'ReadTile']}


def build(output):
    menu = output / 'menus/prefabs/RobCoEyebot/wrist.xml'
    menu.parent.mkdir(parents=True, exist_ok=True)
    for name in ('wrist.xml','filthy_tab.xml','full_filthy.xml'):
        shutil.copyfile(Path(__file__).with_name(name), menu.with_name(name))
    source = Path(__file__).with_name('scripts')
    for domain, names in DEVICE_FUNCTIONS.items():
        destination = output / 'NVSE/user_defined_functions/FilthyCasual' / domain
        destination.mkdir(parents=True, exist_ok=True)
        for name in names:
            shutil.copyfile(source / domain / (name + '.txt'), destination / (name + '.txt'))
    textures = output / 'textures/RobCoEyebot'
    textures.mkdir(parents=True, exist_ok=True)
    Image.new('RGB', (4, 4), (4, 15, 6)).save(textures / 'wrist-backing.dds')
    data = N.Data(version=0x14020007, user_version=11, user_version_2=34)
    data.header.endian_type = 1
    data.header.export_info.creator = b'Filthy Casual TTR'
    root = N.NiNode()
    root.name, root.flags, root.scale = b'FCRecorderOptics', 14, 1
    root.rotation.set_identity()
    data.roots = [root]
    lens = N.NiNode()
    lens.name = b'RobCoEyebotLens'
    lens.flags = 14
    lens.scale = 1
    lens.rotation.set_identity()
    lens.rotation.m_11 = lens.rotation.m_22 = -1
    vector(lens.translation, (0, -9.8, 39.8))
    root.add_child(lens)
    camera = N.NiNode()
    camera.name = b'RobCoEyebotCamera'
    camera.flags = 14
    camera.scale = 1
    # Gamebryo NiCamera columns are forward, up, right.
    camera.rotation.m_11 = camera.rotation.m_22 = camera.rotation.m_33 = 0
    camera.rotation.m_21 = -1
    camera.rotation.m_32 = 1
    camera.rotation.m_13 = -1
    vector(camera.translation, (0, -9.8, 39.8))
    root.add_child(camera)
    root.add_child(screen(output))

    mesh = output / 'meshes/RobCoEyebot/Recorder.nif'
    mesh.parent.mkdir(parents=True, exist_ok=True)
    with mesh.open('wb') as handle:
        data.write(handle)
    verified = N.Data()
    with mesh.open('rb') as handle:
        verified.read(handle)
    if not any(getattr(block, 'name', b'') == lens.name for block in verified.get_global_iterator()):
        raise ValueError('Serialized Eyebot model has no lens mount')

    header = sub('HEDR', struct.pack('<fII', 1.34, 4, 0x804))
    header += sub('CNAM', z('Nikami')) + sub('MAST', z('FalloutNV.esm')) + sub('DATA', bytes(8))
    body = sub('EDID', z('RobCoEyebotRecorder'))
    body += sub('OBND', struct.pack('<6h', -40, -52, -10, 40, 56, 68))
    body += sub('FULL', z('RobCo Eyebot')) + sub('MODL', z(r'Clutter\RobotsAntique\Eyebot_Antique.nif'))
    activator = group(b'ACTI', 0, record('ACTI', 0x01000800, body))
    item = sub('EDID', z('RobCoEyebotRemote')) + sub('OBND', struct.pack('<6h', -6,-4,-8,6,4,8))
    item += sub('FULL', z('RobCo Eyebot Remote')) + sub('MODL', z(r'Clutter\HandRadio\HandRadio.nif'))
    item += sub('ICON', z(r'Interface\Icons\PipboyImages\Items\item_junk.dds')) + sub('DATA', struct.pack('<If', 0, 0.))
    remote = record('MISC', 0x01000801, item)
    room_id = 0x01000803
    room = record('CELL', room_id, sub('EDID', z('RobCoEyebotStorage')) + sub('DATA', b'\x01'))
    fields = sub('EDID', z('RobCoEyebotRef')) + sub('NAME', struct.pack('<I', 0x01000800))
    fields += sub('DATA', struct.pack('<6f', 0,0,0,0,0,0))
    device = record('REFR', 0x01000802, fields, 0xC00)
    room += group(room_id, 6, group(room_id, 8, device))
    cells = group(b'CELL', 0, group(0x803 % 10, 2, group((0x803 // 10) % 10, 3, room)))
    plugin = record('TES4', 0, header) + activator + group(b'MISC', 0, remote) + cells
    (output / 'RobCo Eyebot.esp').write_bytes(plugin)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-data', type=Path, help=argparse.SUPPRESS)  # older developer invocations
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    build(args.output)
