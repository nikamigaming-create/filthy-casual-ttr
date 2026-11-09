from pathlib import Path
import json,struct,shutil,subprocess,os
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'build/pool-vr/mod'
OUT.mkdir(parents=True,exist_ok=True);(ROOT/'evidence').mkdir(exist_ok=True)
shutil.copytree(ROOT/'pool-vr/scripts',OUT/'NVSE/user_defined_functions',dirs_exist_ok=True)
subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(ROOT/'pool-vr/Prepare-Pool.ps1'),'-OriginalModPath',os.environ['POOL_ORIGINAL_MOD'],'-Destination',str(OUT)],check=True)
# New records only: an additional top-level Sunny dialogue topic associated with
# the existing running Goodsprings dialogue quest. No vanilla record override.
def sub(t,b):return t.encode()+struct.pack('<H',len(b))+b
def z(s):return s.encode('cp1252')+b'\0'
def rec(t,id,b,flags=0):return struct.pack('<4sIIIIHH',t.encode(),len(b),flags,id,0,15,0)+b
def group(label,typ,b):return struct.pack('<4sI4siHHHH',b'GRUP',len(b)+24,label,typ,0,0,0,0)+b
def cond(fn,param):return struct.pack('<B3sfH2sIIII',0,b'\0'*3,1.,fn,b'\0'*2,param,0,0,0)
topic=0x01000800;info=0x01000801;quest=0x01000804
bank=json.loads((ROOT/'build/pool-vr/pose-bank.json').read_text())
header=sub('HEDR',struct.pack('<fII',1.34,5+len(bank),0x900+len(bank)))+sub('CNAM',z('Nikami'))+sub('MAST',z('FalloutNV.esm'))+sub('DATA',bytes(8))
tb=sub('EDID',z('FCPoolSunnyChallenge'))+sub('QSTI',struct.pack('<I',quest))+sub('FULL',z('Care for a game of pool?'))+sub('PNAM',struct.pack('<f',55))+sub('DATA',bytes([0,2]))
ib=sub('DATA',bytes([0,0,1,0]))+sub('QSTI',struct.pack('<I',quest))+sub('TPIC',struct.pack('<I',topic))+sub('PNAM',bytes(4))+sub('TRDT',struct.pack('<III B3sI B3s',0,50,0,1,bytes(3),0,1,bytes(3)))+sub('NAM1',z("Sure. Let's play."))+sub('NAM2',b'\0')+sub('NAM3',b'\0')+sub('CTDA',cond(72,0x104E84))+sub('CTDA',cond(67,0x106185))+sub('TCLF',struct.pack('<I',0xC8))+sub('SCHR',bytes.fromhex('0000000000000000000000000000000000000100'))+sub('NEXT',b'')+sub('SCHR',bytes.fromhex('0000000000000000000000000000000000000100'))
# Own travel package. Native record layout checked against GSSunny travel
# packages in the user's master. No scripts, conditions or quest edits copied.
pb=sub('EDID',z('FCPoolSunnyTravel'))+sub('PKDT',struct.pack('<IB3sI',0x00208202,6,bytes(3),0))+sub('PLDT',struct.pack('<III',0,0,0))+sub('PSDT',bytes.fromhex('ffff00ff00000000'))
for event in ('POBA','POEA','POCA'):
    pb+=sub(event,b'')+sub('INAM',bytes(4))+sub('SCHR',bytes.fromhex('0000000000000000000000000000000000000100'))+sub('TNAM',bytes(4))
idle=sub('EDID',z('FCPoolSunnyStroke'))+sub('MODL',z('Characters\\_Male\\IdleAnims\\FilthyCasualPool\\SpecialIdle_FCPool_D110_X5.kf'))+sub('ANAM',bytes(8))+sub('DATA',bytes([0x47])+bytes(7))
idle_records=rec('IDLE',0x01000803,idle)
for pose in bank:
    body=sub('EDID',z('FCPoolStroke'+str(pose['form'])))+sub('MODL',z(pose['path']))+sub('ANAM',bytes(8))+sub('DATA',bytes([0x47])+bytes(7))
    idle_records+=rec('IDLE',0x01000000+pose['form'],body)
quest_record=rec('QUST',quest,sub('EDID',z('FCPoolDialogueQuest'))+sub('FULL',z('Filthy Casual Pool'))+sub('DATA',struct.pack('<BB2sf',1,55,bytes(2),0.)))
# Quest must be registered before its topic info associations are loaded.
(OUT/'Filthy Casual Pool.esp').write_bytes(rec('TES4',0,header)+group(b'QUST',0,quest_record)+group(b'IDLE',0,idle_records)+group(b'PACK',0,rec('PACK',0x01000802,pb))+group(b'DIAL',0,rec('DIAL',topic,tb)+group(struct.pack('<I',topic),7,rec('INFO',info,ib))))

print('Staged VR scripts and Sunny dialogue:',OUT)
