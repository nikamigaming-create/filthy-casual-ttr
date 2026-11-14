"""Build a current, complete showcase edit from actual native-action timelines."""
from pathlib import Path
import json,hashlib,cv2
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'outputs/vehicle-showcase';GUIDE=ROOT/'outputs/casual-guide'
clips=[];sources=[]
def load(name,expected_pid=None,circle=False):
 path=GUIDE/(name+'-stereo.mp4');meta=json.loads(path.with_suffix('.json').read_text());prov=json.loads((GUIDE/(name+'-provenance.json')).read_text(encoding='utf-8-sig'));events=json.loads((GUIDE/(name+'-timeline.json')).read_text(encoding='utf-8-sig'))
 assert meta['ffmpegExit']==0 and meta['fps']==30 and meta['pid']==prov['pid']
 if expected_pid:assert meta['pid']==expected_pid, ('Stale capture',name,meta['pid'],expected_pid)
 finished=[e['seconds'] for e in events if e['action']=='Finished'];assert len(finished)==1
 duration=meta['encodedFrames']/meta['fps'];assert duration>=finished[0], ('Truncated take',name,duration,finished[0])
 if circle:
  done=[e for e in events if e['action'].startswith('Completed viewpoint ')];assert len(done)==8 and done[-1]['action']=='Completed viewpoint 360'
 cap=cv2.VideoCapture(str(path));assert cap.isOpened();assert int(cap.get(cv2.CAP_PROP_FRAME_COUNT))==meta['encodedFrames'];cap.release()
 sources.append(dict(file=str(path),pid=meta['pid'],sha256=hashlib.sha256(path.read_bytes()).hexdigest(),duration=duration,completed=finished[0],repeated=meta['repeatedFrames']))
 return path,events,min(duration,finished[0]+.6)
def add(path,a,b,title,subtitle,chapter):
 if b-a<.2:return
 clips.append(dict(file=path.relative_to(ROOT).as_posix(),start=round(a*30)/30,end=round(b*30)/30,title=title,subtitle=subtitle,chapter=chapter))
def event(events,name):return next(e['seconds'] for e in events if e['action']==name)
car,ev,end=load('roadster-hands-on-wheel',45572)
segments=[(0,'TAKE THE WHEEL','Both hands on the wheel.','01 / DRIVE'),(event(ev,'Drive with Quest controls'),'ON THE ROAD','Right trigger: go. Left trigger: brake.','01 / DRIVE'),(event(ev,'Seated driver footwell'),'SETTLE IN','Seated legs. Hands on the controls.','01 / DRIVE'),(event(ev,'Leave the vehicle'),'STEP OUT','Back on foot with the normal exit control.','01 / DRIVE')]
for i,(start,title,sub,ch) in enumerate(segments):add(car,start,segments[i+1][0] if i+1<len(segments) else end,title,sub,ch)
# Keep completed walkaround/boarding; resumed flight must use the same verified build.
name='vertibird-walkaround-before';first=json.loads((GUIDE/(name+'-provenance.json')).read_text(encoding='utf-8-sig'));pid=first['pid'];assert pid not in (21776,44608), 'Historical aircraft take is not current proof'
for name,title,sub,ch,circle in [
 ('vertibird-walkaround-before','THE CLASSIC VERTIBIRD','A slow look from every side.','02 / WALKAROUND',True),
 ('vertibird-board-cockpit-flight','CLIMB ABOARD','Enter the cabin and take the pilot seat.','03 / BOARD',False),
 ('vertibird-fallout-flight-tour','THE FLIGHT DECK','Look around. Lift off. Return to the Mojave.','04 / FLY',False),
 ('vertibird-exit-walkaround-after','BACK ON THE GROUND','Step out and walk around the landed aircraft.','05 / RETURN',True)]:
 expected=pid if name in ('vertibird-walkaround-before','vertibird-board-cockpit-flight') else None
 path,ev,end=load(name,expected,circle)
 actual=json.loads((GUIDE/(name+'-provenance.json')).read_text(encoding='utf-8-sig'))
 assert actual['nativeSHA256']==first['nativeSHA256'] and actual['addonSHA256']==first['addonSHA256'], 'Aircraft build changed between takes'
 if name=='vertibird-fallout-flight-tour':
  divisions=[(0,title,'Original Fallout instruments and materials.'),(event(ev,'Climb'),'LIFT OFF','Climb above the Mojave.'),(event(ev,'Forward flight'),'TAKE TO THE SKY','Native flight with a tracked cockpit view.'),(event(ev,'Land'),'BRING IT HOME','The original landing sequence takes you back.')]
  for i,(a,t,st) in enumerate(divisions):add(path,a,divisions[i+1][0] if i+1<len(divisions) else end,t,st,ch)
 else:add(path,0,end,title,sub,ch)
(OUT/'edit.json').write_text(json.dumps(dict(output='Filthy-Casual-Classic-Road-and-Rotor-Mobile.mp4',clips=clips),indent=2))
(OUT/'showcase-source-validation.json').write_text(json.dumps(dict(aircraft_pid=pid,sources=sources,total_seconds=sum(c['end']-c['start'] for c in clips)),indent=2))
print('Validated full showcase',round(sum(c['end']-c['start'] for c in clips),2),'seconds; aircraft PID',pid)
