from pathlib import Path
import struct, subprocess, argparse, shutil
a=argparse.ArgumentParser();a.add_argument("--texconv",default=shutil.which("texconv"));args=a.parse_args()
if not args.texconv:raise SystemExit("Pass --texconv PATH to DirectXTex texconv.exe")
root=Path(__file__).resolve().parents[1]
out=root/'build/glass-assets'; tex=out/'textures/FilthyCasual/Glass'; tex.mkdir(parents=True,exist_ok=True)
subprocess.run([args.texconv,'-y','-w','512','-h','512','-f','BC3_UNORM','-m','0','-o',str(tex),str(root/'assets/bottles/glass-impact-v1.png')],check=True)
def sub(t,b):return t.encode()+struct.pack('<H',len(b))+b
def record(t,b,form=0):return t.encode()+struct.pack('<IIIIHH',len(b),0,form,0,15,0)+b
header=sub('HEDR',struct.pack('<fII',1.34,1,0x801))+sub('CNAM',b'Filthy Casuals\0')+sub('MAST',b'FalloutNV.esm\0')+sub('DATA',bytes(8))
b=sub('EDID',b'FCGlassImpactTextureSet\0')+sub('OBND',bytes(12))+sub('TX00',b'FilthyCasual\\Glass\\glass-impact-v1.dds\0')+sub('TX01',b'FilthyCasual\\Glass\\glass-flat_n.dds\0')+sub('DNAM',struct.pack('<H',0))
r=record('TXST',b,0x01000800)
g=b'GRUP'+struct.pack('<I',24+len(r))+b'TXST'+struct.pack('<IHHHH',0,0,0,0,0)+r
(out/'Filthy Casual Glass.esp').write_bytes(record('TES4',header)+g)
print(out)

# Original technical flat normal/specular texture, no game assets. Legacy RGBA DDS.
h=[124,0x100F,4,4,16,0,0]+[0]*11+[32,0x41,0,32,0xFF,0xFF00,0xFF0000,0xFF000000,0x1000,0,0,0,0]
assert len(h)==31
(tex/'glass-flat_n.dds').write_bytes(b'DDS '+struct.pack('<31I',*h)+bytes([128,128,255,16])*16)
