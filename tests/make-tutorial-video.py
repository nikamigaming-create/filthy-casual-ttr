"""Build the portrait reload field guide from current native final-eye takes.
Original comic captions only. Preserve gameplay speed, eyes, and source frames.
A local draft is not publication approval; acceptance remains in the manifest.
"""
from pathlib import Path
import argparse,importlib.util,sys,json,math,subprocess,shutil,hashlib
R=Path(__file__).resolve().parents[1];SRC=R/'outputs/tutorial-development';WORK=R/'work/tutorial-video';OUT=R/'outputs/Filthy-Casual-TTR-Reload-Guide-Mobile.mp4'
spec=importlib.util.spec_from_file_location('mobile_style',R/'tests/make-mobile-montage.py');style=importlib.util.module_from_spec(spec);sys.modules[spec.name]=style;spec.loader.exec_module(style)
style.WORK=WORK
CHAPTERS=[
 ('tutorial-activation','START HERE','A LITTLE\nGUIDANCE.','Optional help, right on the weapon.','Triple-tap LEFT MENU: guide\nOne tap: change reload mode'),
 ('pistol-dry-reload','01 / PISTOL','MAGAZINE.\nTHEN SLIDE.','Point, eject, insert, then chamber.','A: eject | GRIP: spare\nOff-hand TRIGGER: slide'),
 ('charging-handle-reload','02 / RIFLE','FIND THE\nCHARGING HANDLE.','Same magazine lesson. Different moving part.','A: eject | GRIP: spare\nOff-hand TRIGGER: handle'),
 ('bolt-close-hold','03 / BOLT ACTION','WORK\nTHE BOLT.','Point first. Pull back and return.','Off-hand TRIGGER at the bolt\nPull back, return, release'),
 ('cylinder-native-probe','04 / REVOLVER','SWING OUT.\nLOAD UP.','Use the cylinder and speedloader.','Hold A + wrist flick\nGRIP: loader from pouch'),
 ('round-load-WeapNV357Revolver','05 / LOADING GATE','ONE ROUND\nAT A TIME.','Open the gate. Bring a round to the port.','Hold A + wrist flick\nGRIP: round from pouch'),
 ('round-load-WeapNVCaravanShotgun','06 / BREAK ACTION','OPEN. LOAD.\nCLOSE.','Watch the breech and the shell position.','Hold A + wrist flick\nGRIP: shell from pouch'),
 ('round-load-WeapNVHuntingShotgun','07 / PUMP ACTION','WORK THE PUMP.\nTOP UP THE TUBE.','Cycle after firing. Add shells at the highlighted port.','Hold A with the gun hand\nOff-hand GRIP: pump'),
 ('energy-cell-reload','08 / ENERGY CELL','A CELL.\nNOT A SLIDE.','Align the cell with its actual socket.','A: eject cell | GRIP: spare\nInsert, then release GRIP'),
 ('charger-clear-contact','09 / STRIPPER CLIP','FEED FROM\nTHE TOP.','The clip fills an internal magazine.','GRIP: clip from pouch\nOff-hand TRIGGER: slide'),
 ('lever-grip-probe','10 / LEVER ACTION','WORK\nTHE LEVER.','Point to the action. Pull and return.','Hold A with the gun hand\nOff-hand GRIP: action'),
 ('pouch-ammo-types','11 / AMMO TYPES','PICK YOUR\nAMMUNITION.','Use the pouch to change available ammo.','Hold TRIGGER at the pouch\nfor one second'),
 ('pouch-recovery','12 / KEEP YOUR MAG','DROPPED IT?\nPICK IT UP.','Recover, reload, or return it to the pouch.','Native pickup and gravity gloves\nRelease GRIP at the pouch to stow'),
]
def run(args):subprocess.run(args,check=True,timeout=900)
def footer(text,path):
 from PIL import Image,ImageDraw
 im=Image.new('RGBA',(1000,231),(0,0,0,0));d=ImageDraw.Draw(im);d.rounded_rectangle((0,0,999,230),radius=24,fill=style.INK)
 style.text_block(d,text,(30,30,940,175),42,color=style.CREAM,bold=True,gap=12);im.save(path)
def action_text(name,event):
 e=event['action']
 # Preserve the instruction corrections reviewed in the delivered phone edit.
 finishes={
  'tutorial-activation':'Guide enabled. Choose your reload mode.',
  'bolt-close-hold':'Cycle the bolt after each shot.',
  'lever-grip-probe':'Cycle the action after each shot.',
  'pouch-ammo-types':'Ammunition type selected.',
  'pouch-recovery':'Magazine stowed. Ready for the next reload.',
 }
 if e=='Finished' and name in finishes:return finishes[name]
 if name=='pouch-ammo-types' and e=='The ammunition type changes':
  return 'Selected in this take: 10mm, Hollow Point.'
 if e in ('Open the breech or gate','Close the action with the native wrist motion'):
  verb='open' if e.startswith('Open') else 'close'
  part='gate' if name=='round-load-WeapNV357Revolver' else 'breech'
  return f'Hold A and flick your wrist to {verb} the {part}.'
 if e=='Return to the normal hold and close the cylinder':
  return 'Hold A and flick your wrist to close the cylinder.'
 if name=='energy-cell-reload' and e=='Seat magazine through native insertion':
  return 'Push the cell into its socket, then release GRIP.'
 recovery={
  'Original gravity-glove pickup':'Point at the magazine. Hold GRIP and flick back.',
  'Recovered in normal pickup mode':'Hold GRIP near the magazine to pick it up.',
  'Recover for the ammo pouch':'Point, hold GRIP, and flick back to recover it.',
 }
 if name=='pouch-recovery' and e in recovery:return recovery[e]
 if name=='charging-handle-reload':e=e.replace('slide','charging handle').replace('Slide','Charging handle')
 if name=='energy-cell-reload':e=e.replace('magazine','energy cell').replace('spare','cell')
 replacements={'Finished':'Loaded. The native controls take over.','Reach to pouch and grip spare':'Reach to the pouch. Hold GRIP to lift a spare.', 'Eject using native A':'Press A with the gun hand to eject.', 'Seat magazine through native insertion':'Align with the highlighted well. Push in, then release GRIP.', 'Hold A and grip the fore-end; pull back and push forward':'Hold A with the gun hand. Grip the fore-end; pull back, then push forward.', 'Aim the rounds into the open cylinder':'Align the speedloader with the open cylinder. Move it into place.'}
 replacements.update({
  'Cycle the pump with the native controls':'Hold A. Grip the fore-end; pull back, then return.',
  'Bring the round to the actual loading port':'Bring the round to the highlighted loading port.',
  'Seat shell at the native loading point':'Seat the shell. The chambered gun is ready.',
  'Fire the newly loaded ammunition':'Squeeze the gun-hand trigger to fire.',
  'Return to the normal hold':'Release the spare grip and return to your firing hold.',
  'Hold A with the gun hand and grip the action':'Hold A with the gun hand. Grip the highlighted action.',
  'Return the action, then release grip and A':'Return the action. Release GRIP and A.',
 })
 return replacements.get(e,e)
def plan():
 chapters=[]
 for name,label,headline,body,control in CHAPTERS:
  source=SRC/(name+'-stereo.mp4');events=json.loads((SRC/(name+'-timeline.json')).read_text())
  # Lever has a stationary grip-settling pause before the pull, not an action.
  # Keep the approach, then make a visible cut to the complete native cycle.
  raw=json.loads(source.with_suffix('.json').read_text())
  ranges=[(0,405),(480,raw['encodedFrames'])] if name=='lever-grip-probe' else None
  m=style.metadata(source,frame_ranges=ranges)
  if ranges:
   remapped=[]
   for e in events:
    frame=round(e['seconds']*30);elapsed=0
    for begin,finish in ranges:
     if begin<=frame<finish:
      remapped.append(dict(seconds=(elapsed+frame-begin)/30,action=e['action']));break
     elapsed+=finish-begin
   events=remapped

  if events[-1]['action']!='Finished':raise ValueError('Incomplete native action sequence: '+name)
  end=round(min(m['encodedFrames']/30,events[-1]['seconds']+2.8)*30)/30
  chapters.append(dict(name=name,label=label,headline=headline,body=body,control=control,source=str(source),duration=end,events=events,frameRanges=ranges,sha256=hashlib.sha256(source.read_bytes()).hexdigest(),frames=m['encodedFrames'],pid=m['pid']))
 return chapters

def render(chapters,encoder="libx264"):
 ff=shutil.which('ffmpeg');base=[ff,'-v','error','-y']
 enc=(['-c:v','h264_nvenc','-preset','p5','-rc','vbr','-cq','20','-b:v','0'] if encoder=='h264_nvenc' else ['-c:v','libx264','-preset','veryfast','-crf','18','-threads','4'])+['-pix_fmt','yuv420p','-an'];clips=[]
 title=style.Shot('tutorial-title',None,0,4,'RELOAD\nFIELD GUIDE','For the filthy casuals going manual.','','LEARN IT',card='title')
 card=style.caption_asset(title);clip=WORK/'title.mp4';run(base+['-loop','1','-framerate','30','-i',str(card),'-t','4','-vf','setsar=1']+enc+[str(clip)]);clips.append((clip,4.0))
 for c in chapters:
  cache=WORK/(c['name']+'-render.json');clip=WORK/(c['name']+'.mp4')
  key=hashlib.sha256(json.dumps({'chapter':c,'captions':[action_text(c['name'],e) for e in c['events']],'encoder':encoder,'layout':2},sort_keys=True).encode()).hexdigest()
  if cache.exists() and clip.exists():
   prior=json.loads(cache.read_text())
   if prior.get('key')==key and prior.get('sha256')==hashlib.sha256(clip.read_bytes()).hexdigest():
    clips.append((clip,c['duration']));print('Reused '+c['name'],flush=True);continue
  shot=style.Shot(c['name'],c['source'],0,c['duration'],c['headline'],c['body'],c['control'],c['label'])
  card=style.caption_asset(shot);args=base+['-loop','1','-framerate','30','-i',str(card),'-i',c['source']]
  filters=[];video='1:v'
  if c['frameRanges']:
   spans=c['frameRanges'];labels=''.join(f'[src{i}]' for i in range(len(spans)))
   filters.append(f'[1:v]split={len(spans)}'+labels)
   for i,(begin,end) in enumerate(spans):filters.append(f'[src{i}]trim=start_frame={begin}:end_frame={end},setpts=PTS-STARTPTS[cut{i}]')
   filters.append(''.join(f'[cut{i}]' for i in range(len(spans)))+f'concat=n={len(spans)}:v=1:a=0[edited]');video='edited'
  filters.extend([f'[{video}]setpts=PTS-STARTPTS,crop=1032:1104:1032:0,scale=1000:1070,setsar=1[g]','[0:v][g]overlay=40:390:shortest=1[base]']);last='base'

  for i,e in enumerate(c['events']):
   start=round(float(e['seconds'])*30)/30;end=round(float(c['events'][i+1]['seconds'])*30)/30 if i+1<len(c['events']) else c['duration']
   if end<=start:continue
   p=WORK/(c['name']+f'-step-{i}.png');footer(action_text(c['name'],e),p);args+=['-loop','1','-framerate','30','-i',str(p)];idx=2+i;target=f'step{i}'
   filters.append(f"[{last}][{idx}:v]overlay=40:1504:enable='gte(t,{start:.6f})*lt(t,{end:.6f})'[{target}]");last=target
  clip=WORK/(c['name']+'.mp4');run(args+['-filter_complex_threads','2','-filter_complex',';'.join(filters),'-map',f'[{last}]','-t',str(c['duration']),'-r','30']+enc+[str(clip)]);clips.append((clip,c['duration']));cache.write_text(json.dumps({'key':key,'sha256':hashlib.sha256(clip.read_bytes()).hexdigest()}));print('Rendered '+c['name'],flush=True)
 args=base.copy()
 for clip,_ in clips:args+=['-i',str(clip)]
 last='0:v';at=clips[0][1]-.4;filters=[]
 for i in range(1,len(clips)):
  target=f'wipe{i}';filters.append(f'[{last}][{i}:v]xfade=transition=wipeleft:duration=0.4:offset={at:.6f}[{target}]');last=target;at+=clips[i][1]-.4
 run(args+['-filter_complex_threads','2','-filter_complex',';'.join(filters),'-map',f'[{last}]']+enc+['-movflags','+faststart',str(OUT)])

if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--render',action='store_true');ap.add_argument('--encoder',choices=['libx264','h264_nvenc'],default='libx264');args=ap.parse_args();WORK.mkdir(parents=True,exist_ok=True);chapters=plan()
 manifest={'captionRevision':2,'output':str(OUT),'resolution':[1080,1920],'fps':30,'gameplaySpeed':1,'gameplayColorGrade':'none','eye':'complete right eye; stereo originals retained','audio':False,'interpolation':False,'capture':'native final-eye recordings, choreographed XR input','reviewStatus':'pending full edit and remaining source review','chapters':chapters}
 (WORK/'edit-plan.json').write_text(json.dumps(manifest,indent=2))
 if args.render:
  render(chapters,args.encoder);manifest['encoder']=args.encoder;manifest['outputSHA256']=hashlib.sha256(OUT.read_bytes()).hexdigest();(WORK/'edit-manifest.json').write_text(json.dumps(manifest,indent=2));print(OUT)
 else:print('Prepared tutorial and shared-ammunition edit plan; no encoding requested.')
