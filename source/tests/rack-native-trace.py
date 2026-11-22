"""Bounded local read-only C565 rack-pickup observation. Never sends game input.
Candidate/pull snapshots are asynchronous observations, not an atomic engine frame.
"""
import argparse, ctypes as C, hashlib, json, math, os, struct, time
from ctypes import wintypes as W
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HOST_SHA = 'c565a6c296b597f89fcf84d7f6001d47d1635792f0e8cacad92c6a88f6823e73'
EXE_SHA = '518c87f58a6c4d9826e9ef8fbb7f4213882fa70822675610d45aea2464502a57'
EXE_PATH = Path(r'D:\SteamLibrary\steamapps\common\Fallout New Vegas\FalloutNV.exe')
HOST_FILE = ROOT / 'work/ttr112/TTWVR.dll'


def digest(path):
    with open(path, 'rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def source_json(path):
    for _ in range(3):
        try:
            return json.loads(path.read_text(encoding='utf-8-sig'))
        except (OSError, json.JSONDecodeError):
            time.sleep(.002)
    return None


def pe_identity(blob):
    if blob[:2] != b'MZ':
        raise ValueError('not PE')
    at = struct.unpack_from('<I', blob, 0x3c)[0]
    if blob[at:at+4] != b'PE\0\0':
        raise ValueError('invalid PE signature')
    machine = struct.unpack_from('<H', blob, at+4)[0]
    stamp = struct.unpack_from('<I', blob, at+8)[0]
    magic = struct.unpack_from('<H', blob, at+24)[0]
    size = struct.unpack_from('<I', blob, at+24+56)[0]
    return dict(machine=machine, timestamp=stamp, magic=magic, size=size)


class MODULEENTRY32W(C.Structure):
    _fields_ = [('dwSize', W.DWORD), ('th32ModuleID', W.DWORD), ('th32ProcessID', W.DWORD),
                ('GlblcntUsage', W.DWORD), ('ProccntUsage', W.DWORD),
                ('modBaseAddr', C.c_void_p), ('modBaseSize', W.DWORD), ('hModule', W.HMODULE),
                ('szModule', W.WCHAR*256), ('szExePath', W.WCHAR*260)]


class Reader:
    def __init__(self, pid):
        self.k = C.WinDLL('kernel32', use_last_error=True)
        k = self.k
        k.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]; k.OpenProcess.restype = W.HANDLE
        k.ReadProcessMemory.argtypes = [W.HANDLE,C.c_void_p,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
        k.ReadProcessMemory.restype = W.BOOL
        k.CloseHandle.argtypes = [W.HANDLE]; k.CloseHandle.restype = W.BOOL
        k.QueryFullProcessImageNameW.argtypes = [W.HANDLE,W.DWORD,W.LPWSTR,C.POINTER(W.DWORD)]
        k.QueryFullProcessImageNameW.restype = W.BOOL
        k.CreateToolhelp32Snapshot.argtypes = [W.DWORD,W.DWORD]; k.CreateToolhelp32Snapshot.restype = W.HANDLE
        for name in ('Module32FirstW','Module32NextW'):
            fn = getattr(k,name); fn.argtypes = [W.HANDLE,C.POINTER(MODULEENTRY32W)]; fn.restype = W.BOOL
        # Query identity and read memory only; no write, operation, thread or debug rights.
        self.h = k.OpenProcess(0x1000|0x10,False,pid)
        if not self.h: raise C.WinError(C.get_last_error())
        name = C.create_unicode_buffer(32768); count=W.DWORD(len(name))
        if not k.QueryFullProcessImageNameW(self.h,0,name,C.byref(count)): raise C.WinError(C.get_last_error())
        self.path=Path(name.value)
        self.modules=[]
        snap=k.CreateToolhelp32Snapshot(0x8|0x10,pid)
        if snap == C.c_void_p(-1).value: raise C.WinError(C.get_last_error())
        try:
            ent=MODULEENTRY32W();ent.dwSize=C.sizeof(ent)
            ok=k.Module32FirstW(snap,C.byref(ent))
            while ok:
                self.modules.append(dict(name=ent.szModule,path=ent.szExePath,base=ent.modBaseAddr,size=ent.modBaseSize))
                ok=k.Module32NextW(snap,C.byref(ent))
        finally: k.CloseHandle(snap)

    def close(self):
        if self.h: self.k.CloseHandle(self.h); self.h=None

    def read(self, address, length):
        if not 0x10000 <= address < 0x100000000 or not 0 < length <= 0x10000:
            raise ValueError('invalid bounded x86 read')
        buf=C.create_string_buffer(length); got=C.c_size_t()
        if not self.k.ReadProcessMemory(self.h,address,buf,length,C.byref(got)) or got.value!=length:
            raise OSError(f'ReadProcessMemory {address:08X}+{length:X} failed ({C.get_last_error()})')
        return buf.raw

    def u32(self,a): return struct.unpack('<I',self.read(a,4))[0]
    def f32(self,a): return struct.unpack('<f',self.read(a,4))[0]
    def refid(self,p):
        try: return self.u32(p+0xc) if p else 0
        except OSError: return None


def candidate_row(raw):
    ref,root,distance,radius,mass=struct.unpack_from('<IIfff',raw)
    return dict(reference=ref,root=root,distance=distance,radius=radius,mass=mass,childFlag=raw[0x14])


def capture(reader,base,rack_id):
    r=reader
    out={}
    count=r.u32(base+0x143C1E0)
    out['candidateCount']=count;out['rackCandidates']=[]
    if count>64: out['candidateError']='count exceeds native64 capacity'
    elif count:
        raw=r.read(base+0x1439DE0,count*0x90)
        for i in range(count):
            row=candidate_row(raw[i*0x90:(i+1)*0x90])
            row['refID']=r.refid(row['reference'])
            if row['refID']==rack_id: row['index']=i;out['rackCandidates'].append(row)
    out['candidateCountStable']=r.u32(base+0x143C1E0)==count
    settings=base+0xF76648
    configured=r.f32(settings+0x334);retail=r.f32(0x011D1234)
    out['massLimit']={'configured':configured,'retailFallback':retail,'effective':configured if configured>0 else retail}
    out['reach']=r.f32(settings+0x28)*r.f32(settings+0x32c)
    out['pulls']=[]
    for hand in range(2):
        p=base+0xEB5A98+hand*0x88; raw=r.read(p,0x88)
        ref,root=struct.unpack_from('<II',raw,4)
        pull={'hand':hand,'active':bool(raw[0]),'reference':ref,'refID':r.refid(ref),'root':root,
              'elapsed':struct.unpack_from('<f',raw,0x10)[0],
              'duration':struct.unpack_from('<f',raw,0x14)[0],
              'radius':struct.unpack_from('<f',raw,0x18)[0],
              'localAnchor':list(struct.unpack_from('<3f',raw,0x1c)),
              'launchPending':bool(raw[0x35])}
        if pull['active'] and root:
            try:
                bound=r.u32(root+0x20)
                pull['worldBound']=list(struct.unpack('<4f',r.read(bound,16)))
                current=r.read(p,12)
                pull['identityStable']=current==raw[:12]
            except OSError as e: pull['error']=str(e)
        out['pulls'].append(pull)
    out['slots']=[]
    for flag,ref,hand in [(0x136CE35,0x136D2F0,0xEB5A88),(0x136CF04,0x136CF0C,0x136CF08)]:
        ptr=r.u32(base+ref)
        out['slots'].append(dict(active=bool(r.read(base+flag,1)[0]),reference=ptr,refID=r.refid(ptr),hand=r.u32(base+hand)))
    return out


def finite_json(value, path='', invalid=None):
    invalid = [] if invalid is None else invalid
    if isinstance(value, float) and not math.isfinite(value):
        invalid.append(path); return None
    if isinstance(value, dict): return {k:finite_json(v,path+'/'+str(k),invalid) for k,v in value.items()}
    if isinstance(value, list): return [finite_json(v,path+'/'+str(i),invalid) for i,v in enumerate(value)]
    return value


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--seconds',type=float,default=10);ap.add_argument('--hz',type=float,default=20)
    ap.add_argument('--session',type=Path,default=ROOT/'sim-session')
    ap.add_argument('--output',type=Path,default=ROOT/'work/pool-equipment/rack-native-trace.json')
    ap.add_argument('--host-file',type=Path,default=HOST_FILE)
    ap.add_argument('--selftest',action='store_true');a=ap.parse_args()
    if a.selftest:
        raw=bytearray(0x90);struct.pack_into('<IIfff',raw,0,123,456,7,17.4,6);raw[20]=1
        row=candidate_row(raw);assert row['reference']==123 and row['mass']==6 and row['childFlag']==1
        assert digest(a.host_file)==HOST_SHA
        assert pe_identity(a.host_file.read_bytes()[:4096])['timestamp']==0x6AF4F66C
        print('PASS: row decoder and exact owned host identity; no process opened');return
    if os.name!='nt' or not 0<a.seconds<=10 or not 1<=a.hz<=20: ap.error('Windows; bounds: seconds<=10, hz<=20')
    state=source_json(a.session/'state.json')
    if not state or time.time()-(a.session/'state.json').stat().st_mtime>5: raise RuntimeError('fresh simulator PID required')
    pid=int(state['pid']);r=Reader(pid)
    result={'pid':pid,'startedUTC':datetime.now(timezone.utc).isoformat(),'readOnly':True,
            'atomicFrame':False,'nativeGrip':'unavailable: no remote calls or unverified memory offsets','samples':[]}
    try:
        if r.path.resolve()!=EXE_PATH.resolve() or digest(r.path)!=EXE_SHA: raise RuntimeError('unexpected game executable')
        mods=[m for m in r.modules if m['name'].lower()=='ttwvr.dll']
        if len(mods)!=1: raise RuntimeError('expected one TTWVR.dll module')
        mod=mods[0];base=mod['base']
        if digest(a.host_file)!=HOST_SHA: raise RuntimeError('unknown owned TTWVR host binary')
        expected=pe_identity(a.host_file.read_bytes()[:4096]);actual=pe_identity(r.read(base,4096))
        if actual!=expected or expected['machine']!=0x14c or expected['magic']!=0x10b: raise RuntimeError('loaded host PE mismatch')
        # The verified immutable instruction bytes at the exact native entry.
        if r.read(base+0x1A7700,7)!=bytes.fromhex('e86b68feff84c0'): raise RuntimeError('loaded native pickup signature mismatch')
        if r.read(base+0x1A039C,6)!=bytes.fromhex('b001c332c0c3'): raise RuntimeError('loaded shared-slot signature mismatch')
        mapped=Path(mod['path'])
        mapped_hash=digest(mapped) if mapped.is_file() else None
        if mapped_hash and mapped_hash!=HOST_SHA: raise RuntimeError('mapped module file hash mismatch')
        result['identity']={'exe':str(r.path),'exeSHA256':EXE_SHA,'host':mod,'ownedHost':str(a.host_file),'hostSHA256':HOST_SHA,'mappedFileSHA256':mapped_hash,'loadedPE':actual}
        equipment=source_json(a.session/'pool-equipment.json')
        rack_id=int(equipment['items'][4]['ref'])
        if rack_id&0xffffff!=0x910: raise RuntimeError('equipment index4 is not authored rack0910')
        result['rackRefID']=rack_id
        start=time.perf_counter();samples=int(math.ceil(a.seconds*a.hz))
        for index in range(samples):
            wait=start+index/a.hz-time.perf_counter()
            if wait>0:time.sleep(wait)
            sample={'t':time.perf_counter()-start,'utc':datetime.now(timezone.utc).isoformat()}
            try: sample.update(capture(r,base,rack_id))
            except (OSError,ValueError) as e:sample['error']=str(e)
            for filename,key in [('state.json','sim'),('addon-state.json','addon'),('pool-equipment.json','equipment')]:
                value=source_json(a.session/filename)
                if key=='sim' and value and value.get('pid')!=pid:raise RuntimeError('PID changed during capture')
                if value:
                    fields={'sim':('pid','frame','handL','handR','head','controls'),
                            'addon':('pid','gloves','hands','palms','nativeGrabReach','nativeGrabHand','nativeGrabRef'),
                            'equipment':('palms',)}[key]
                    sample[key]={field:value.get(field) for field in fields}
                    if key=='equipment':sample[key]['rack']=value['items'][4]
                    sample[key]['fileAge']=time.time()-(a.session/filename).stat().st_mtime
            result['samples'].append(sample)
        result['elapsed']=time.perf_counter()-start
    finally:
        r.close()
        a.output.parent.mkdir(parents=True,exist_ok=True)
        invalid=[]; result=finite_json(result,invalid=invalid); result['nonFiniteFields']=invalid
        a.output.write_text(json.dumps(result,indent=2,allow_nan=False),encoding='utf-8')
    print(json.dumps({'output':str(a.output),'pid':pid,'samples':len(result['samples']),'elapsed':result.get('elapsed')}))

if __name__=='__main__':main()
