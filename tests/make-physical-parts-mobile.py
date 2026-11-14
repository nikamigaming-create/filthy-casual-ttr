"""Mobile edit of reviewed native physical-part captures; no generated gameplay."""
from pathlib import Path
import importlib.util,sys,json
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('mobile_style',ROOT/'tests/make-mobile-montage.py')
m=importlib.util.module_from_spec(spec);sys.modules[spec.name]=m;spec.loader.exec_module(m)
m.SRC=ROOT/'outputs/physical-parts';m.WORK=m.SRC/'mobile-edit';m.OUT=ROOT/'outputs/Filthy-Casual-Physical-Parts-Mobile.mp4'
m.WORK.mkdir(parents=True,exist_ok=True)
shots=[m.Shot('title',None,0,3,'DROPPED?\nGRAB IT.','Filthy Casual TTR / physical reload parts.','','REAL PARTS',card='title')]
for key,take,begin,end,title,body,control in [
 ('rifle-drop','carbine',8.5,17.5,'RIFLE MAGS.\nREAL PHYSICS.','Assault carbine / manual mode.','Drop a spare. It stays in the world.'),
 ('cell-drop','plasma',6,15,'ENERGY CELLS\nCOUNT TOO.','Plasma Defender / same native controls.','No new throw button to learn.'),
 ('cell-grab','plasma',18,25.5,'PICK IT\nBACK UP.','Your original grip control.','Squeeze grip to recover the dropped part.'),
 ('rifle-grab','carbine',30,35.5,'BACK IN\nYOUR HAND.','Native pickup / magazine recovered.','X Take returns its rounds to inventory.'),
 ('rifle-throw','carbine',39.5,44.5,'GIVE IT\nA TOSS.','Move your hand. Release your grip.','The original throw controls still apply.'),
 ('cell-throw','plasma',30.5,36,'SAME DEAL.\nSTAY CASUAL.','Energy cells can be thrown too.','Manual and automatic reload remain switchable.')]:
 source=m.SRC/(take+'-table-stereo.mp4');md=m.metadata(source)
 if md['pid']!=33252:raise ValueError('Unexpected unreviewed source process')
 if end>md['encodedFrames']/md['fps']:raise ValueError('Cut exceeds capture')
 shots.append(m.footage(key,source,begin,end,title,body,control,'KEEP IT!'))
manifest={'format':'1080x1920 / 30fps / full right eye / original speed','audio':False,'gamma':1.22,'shots':[m.asdict(s) for s in shots],
 'note':'Actual native gameplay with choreographed XR input. Visible wipes separate selected takes; not an uninterrupted first-attempt pickup claim. Both-eye masters retained; phone edit is the right eye. No generated gameplay frames.',
 'sources':[{ 'path':str(p),'sha256':m.sha256(p),'metadata':str(p.with_suffix('.json'))} for p in sorted({Path(s.source) for s in shots if s.source})]}
(m.WORK/'edit-plan.json').write_text(json.dumps(manifest,indent=2))
for shot in shots:m.caption_asset(shot)
if '--render' in sys.argv:
 m.render(shots);manifest['outputSha256']=m.sha256(m.OUT);(m.WORK/'edit-manifest.json').write_text(json.dumps(manifest,indent=2));print(m.OUT)