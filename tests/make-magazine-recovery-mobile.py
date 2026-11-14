"""Reviewed native magazine-recovery edit; original speed, full right eye."""
from pathlib import Path
import importlib.util,sys,json,subprocess,shutil
r=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('recovery_style',r/'tests/make-mobile-montage.py');m=importlib.util.module_from_spec(spec);sys.modules[spec.name]=m;spec.loader.exec_module(m)
m.WORK=r/'outputs/magazine-recovery/mobile-edit';m.WORK.mkdir(parents=True,exist_ok=True)
out=r/'outputs/Filthy-Casual-Magazine-Recovery-Mobile.mp4'
shots=[m.Shot('title',None,0,3,'DROPPED IT?\nKEEP IT.','Filthy Casual TTR / manual reload recovery.','','NO WASTE',card='title')]
for key,take,start,end,title,body,control,stamp in [
 ('eject','rifle',1,7.5,'REAL MAGS.\nREAL PHYSICS.','A dropped magazine stays in the world.','Manual mode / original TTR ejection.','KEEP IT!'),
 ('gravity','rifle',9,16.5,'POINT. GRIP.\nFLICK BACK.','Your original gravity gloves still work.','Hold grip and flick back to pull it in.','COME HERE'),
 ('rifle-insert','rifle',22,34,'BACK IN\nTHE GUN.','Recovered magazine. Native manual reload.','Keep gripping. Align the magazine. Insert.','CLICK!'),
 ('rifle-fire','rifle',35,40,'READY\nTO FIRE.','The recovered magazine feeds the gun.','Use your normal firing control.','BACK AT IT'),
 ('cell-insert','plasma',26.5,34,'ENERGY\nCELLS TOO.','Plasma Defender / same native reload path.','Recover. Align. Insert.','CHARGED'),
 ('cell-fire','plasma',35,39.5,'KEEP YOUR\nREMAINING AMMO.','Partial magazines retain their rounds.','Empty magazines stay empty.','NO MAGIC'),
 ('pouch-approach','plasma',52.5,55,'OR RETURN IT\nTO THE POUCH.','Keep the part in your hand as you approach.','Bring it to your ammo pouch.','STOW IT'),
 ('pouch-release','plasma',59.5,64.5,'RELEASE GRIP.\nSTAY CASUAL.','Normal pickup. Normal manual reload.','Release grip at your ammo pouch.','LESS FUSS')]:
 src=r/'outputs/magazine-recovery'/f'{take}-stereo.mp4';md=m.read_json(src.with_suffix('.json'))
 assert md['pid']==({'rifle':28448,'plasma':28476}[take])
 assert end<=md['encodedFrames']/md['fps']
 assert md['ffmpegExit']==0 and (md['width'],md['height'],md['fps'])==(2064,1104,30)
 assert len(md['frames'])==md['encodedFrames']
 assert not any(f['repeated'] for f in md['frames'][round(start*30):round(end*30)]), 'Repeated frame inside selected span'

 shots.append(m.footage(key,src,start,end,title,body,control,stamp))
manifest={'format':'1080x1920 H.264 /30fps /faststart','audio':False,'speed':1,'eye':'complete right eye','gamma':1.35,'shots':[m.asdict(s) for s in shots],
 'sources':[{'path':str(p),'sha256':m.sha256(p),'metadata':str(p.with_suffix('.json'))} for p in sorted({Path(s.source) for s in shots if s.source})],
 'provenance':'Actual game physics and original reload states, choreographed XR input. Both-eye masters retained. Visible wipes mark selected segments. No generated gameplay/interpolated frames. Rifle after40seconds excluded. Pouch socket and final seating partly obscured by native hand/body; captions describe the proven controls, not an exposed socket view.'}
(m.WORK/'edit-plan.json').write_text(json.dumps(manifest,indent=2))
ff=shutil.which('ffmpeg');base=[ff,'-hide_banner','-loglevel','error','-y'];enc=['-c:v','libx264','-preset','veryfast','-crf','18','-pix_fmt','yuv420p','-threads','4','-an']
for s in shots:
 card=m.caption_asset(s)
 if '--render' not in sys.argv:continue
 args=base+['-loop','1','-framerate','30','-i',str(card)]
 if s.source:
  args+=['-ss',str(s.start),'-i',s.source,'-filter_complex_threads','2','-filter_complex',f'[1:v]trim=duration={s.duration},setpts=PTS-STARTPTS,crop=1032:1104:1032:0,eq=gamma=1.35,scale=1000:1070,setsar=1[g];[0:v][g]overlay=40:390:shortest=1[v]','-map','[v]']
 else:args+=['-vf','setsar=1']
 subprocess.run(args+['-t',str(s.duration),'-r','30']+enc+[str(m.WORK/f'{s.key}.mp4')],check=True)
 print('Rendered',s.key,flush=True)
if '--render' in sys.argv:
 args=base.copy();f=[];cards=[];n=len(shots)
 for i,s in enumerate(shots):
  args+=['-i',str(m.WORK/f'{s.key}.mp4')];begin=0 if i==0 else .2;end=s.duration if i==n-1 else s.duration-.2
  f += [f'[{i}:v]split=2[c{i}][g{i}]',f'[c{i}]trim=start={begin}:end={end},setpts=PTS-STARTPTS[c{i}t]',f'[g{i}]crop=1000:1070:40:390,setpts=PTS-STARTPTS[g{i}t]'];cards.append(f'[c{i}t]')
 f.append(''.join(cards)+f'concat=n={n}:v=1:a=0[cards]');last='g0t';offset=shots[0].duration-.4
 for i in range(1,n):
  target=f'wipe{i}';trans='wipeleft' if i%2 else 'wiperight';f.append(f'[{last}][g{i}t]xfade=transition={trans}:duration=0.4:offset={offset:.6f}[{target}]');offset+=shots[i].duration-.4;last=target
 f.append(f'[cards][{last}]overlay=40:390:shortest=1[out]')
 subprocess.run(args+['-filter_complex_threads','2','-filter_complex',';'.join(f),'-map','[out]']+enc+['-movflags','+faststart',str(out)],check=True)
 manifest['outputSha256']=m.sha256(out);manifest['transitions']='0.4s gameplay-only wipes, captions switch intact at midpoint';(m.WORK/'edit-manifest.json').write_text(json.dumps(manifest,indent=2));print(out)
