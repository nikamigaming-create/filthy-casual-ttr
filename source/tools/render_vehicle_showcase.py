"""Phone-first showcase edit of actual final-eye gameplay, with original artwork/music."""
from pathlib import Path
import json,math,subprocess,cv2,numpy as np,wave,sys,re
from PIL import Image,ImageDraw,ImageFont
from functools import lru_cache
ROOT=Path(__file__).resolve().parents[1];folder=sys.argv[1] if len(sys.argv)>1 else 'vehicle-showcase';assert re.fullmatch(r'[a-z0-9-]+',folder);OUT=ROOT/'outputs'/folder;OUT.mkdir(parents=True,exist_ok=True)
W,H,FPS=720,1280,30
CREAM=(238,226,193);INK=(25,47,45);ORANGE=(204,91,46);MUTED=(163,185,155)
@lru_cache(maxsize=128)
def font(size,bold=False):return ImageFont.truetype('C:/Windows/Fonts/'+('arialbd.ttf' if bold else 'arial.ttf'),size)
def fit(text,size,maxwidth,bold=False):
 f=font(size,bold)
 while f.getlength(text)>maxwidth:size-=1;f=font(size,bold)
 return f
def make_base(config=None):
 config=config or {};brand=config.get('brand',{})
 if isinstance(brand,str):brand={'title':brand}
 title=brand.get('title','FILTHY CASUAL');subtitle=brand.get('subtitle','TTR  /  ROAD & ROTOR')
 footer=config.get('footer','NIKAMI  /  FILTHY CASUAL TTR')
 im=Image.new('RGB',(W,H),INK);d=ImageDraw.Draw(im)
 # Optional edit.json branding changes artwork only, never source gameplay.
 for y in range(0,H,12):
  for x in range((y//12%2)*6,W,12):d.ellipse((x,y,x+1,y+1),fill=(32,56,51))
 d.rounded_rectangle((20,18,700,142),radius=18,fill=CREAM)
 d.text((41,30),title,font=fit(title,61,625,True),fill=ORANGE,stroke_width=0)
 d.text((38,27),title,font=fit(title,61,625,True),fill=INK)
 d.text((41,103),subtitle,font=fit(subtitle,21,625,True),fill=INK)
 d.line((25,159,695,159),fill=ORANGE,width=5)
 d.rounded_rectangle((12,174,708,957),radius=9,fill=CREAM)
 for x in (22,698):
  for y in (183,948):d.ellipse((x-3,y-3,x+3,y+3),fill=INK)
 if footer:
  d.line((35,1218,685,1218),fill=MUTED,width=2)
  d.text((35,1236),footer,font=fit(footer,18,560,True),fill=CREAM)
  d.text((618,1232),'01',font=font(26,True),fill=ORANGE)
 return im
base=make_base()
@lru_cache(maxsize=256)
def background(title,subtitle,chapter,phase):
 im=base.copy();draw=ImageDraw.Draw(im)
 # Pop into place gently; gameplay never gets obscured by labels.
 dy=round(12*(1-min(1,phase/.45))**3)
 draw.rounded_rectangle((34,981+dy,195,1018+dy),radius=16,fill=ORANGE)
 draw.text((49,988+dy),chapter,font=font(16,True),fill=CREAM)
 yy=1034+dy
 for line in title.split('\n'):
  draw.text((34,yy),line,font=fit(line,46,650,True),fill=CREAM);yy+=51
 draw.text((36,1156+dy),subtitle,font=fit(subtitle,23,646),fill=MUTED)
 return im

def audio(duration):
 sr=48000;n=round(duration*sr);a=np.zeros(n,dtype=np.float32);rng=np.random.default_rng(421);beat=60/104
 def note(at,midi,length,level,timbre):
  start=round(at*sr);count=min(round(length*sr),n-start)
  if count<=0:return
  t=np.arange(count)/sr;f=440*2**((midi-69)/12)
  if timbre=='bass':v=np.sin(2*np.pi*f*t)+.22*np.sin(4*np.pi*f*t);env=(1-np.exp(-t*100))*np.exp(-t*5)
  else:v=np.sin(2*np.pi*f*t)+.22*np.sin(2*np.pi*f*3*t);env=(1-np.exp(-t*150))*np.exp(-t*3.7)
  a[start:start+count]+=level*v*env
 chords=[(48,[60,64,67,69]),(53,[60,65,69,72]),(50,[62,66,69,72]),(55,[62,65,69,71])]
 for bar in range(math.ceil(duration/(4*beat))):
  root,tones=chords[bar%4];start=bar*4*beat
  for k,m in enumerate([root,root+7,root+12,root+7]):note(start+k*beat,m,.48,.16,'bass')
  for k in [1,3]:
   for m in tones:note(start+k*beat,m,.55,.037,'keys')
  if bar%2==0:
   for k,m in enumerate([tones[2]+12,tones[1]+12,tones[0]+12]):note(start+(k*.67+.4)*beat,m,.5,.04,'keys')
  for k in range(8):
   off=round((start+k*.5*beat)*sr);count=min(round(.055*sr),n-off)
   if count>0:
    t=np.arange(count)/sr;noise=rng.normal(0,1,count);noise=np.diff(noise,prepend=0);a[off:off+count]+=.007*noise*np.exp(-t*70)
 a*=np.minimum(np.arange(n)/sr,1)*np.minimum((n-np.arange(n))/sr,1)
 stereo=np.stack([a,a*.97],axis=1);pcm=(np.clip(stereo,-.9,.9)*32767).astype('<i2')
 with wave.open(str(OUT/'original-score.wav'),'wb') as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(sr);f.writeframes(pcm.tobytes())

def main():
 global base
 manifest=json.loads((OUT/'edit.json').read_text());base=make_base(manifest);background.cache_clear();base.save(OUT/'design.png')
 clips=manifest['clips'];duration=sum(c['end']-c['start'] for c in clips);audio(duration)
 dest=OUT/manifest.get('output','Filthy-Casual-Vertibird-Mobile.mp4')
 temporary=dest.with_name(dest.stem+'.rendering.mp4')
 cmd=['ffmpeg','-hide_banner','-loglevel','warning','-y','-f','rawvideo','-pix_fmt','rgb24','-s','720x1280','-r','30','-i','pipe:0','-i',str(OUT/'original-score.wav'),'-c:v','libx264','-preset','fast','-crf','22','-c:a','aac','-b:a','128k','-pix_fmt','yuv420p','-movflags','+faststart','-shortest',str(temporary)]
 proc=subprocess.Popen(cmd,stdin=subprocess.PIPE);total=0
 try:
  for idx,c in enumerate(clips):
   cap=cv2.VideoCapture(str(ROOT/c['file']));sourcefps=cap.get(cv2.CAP_PROP_FPS);assert abs(sourcefps-FPS)<.01, 'Source cadence differs; explicit resampling required';cap.set(cv2.CAP_PROP_POS_FRAMES,round(c['start']*sourcefps));frames=round((c['end']-c['start'])*FPS)
   for j in range(frames):
    ok,frame=cap.read()
    if not ok:raise RuntimeError('Source clip ended early')
    frame=frame[:,:frame.shape[1]//2];frame=cv2.cvtColor(frame,cv2.COLOR_BGR2RGB)
    im=Image.fromarray(frame);scale=min(720/im.width,774/im.height);im=im.resize((round(im.width*scale),round(im.height*scale)),Image.Resampling.LANCZOS)
    canvas=background(c['title'],c['subtitle'],c['chapter'],min(j,14)/FPS).copy();canvas.paste(im,((720-im.width)//2,174+(774-im.height)//2))
    draw=ImageDraw.Draw(canvas);progress=(total+j)/(duration*FPS);draw.rectangle((20,944,20+680*progress,950),fill=ORANGE)
    # Brief printed diagonal wipe between chapters, not an accelerated gameplay cut.
    if idx and j<7:
     edge=round(W*(j/7));draw.polygon([(edge-100,174),(edge,174),(edge-100,957),(edge-200,957)],fill=CREAM)
    proc.stdin.write(canvas.tobytes())
    if j==min(60,frames-1):canvas.save(OUT/f'preview-{idx}.jpg',quality=90)
   total+=frames;cap.release()
 finally:proc.stdin.close()
 rc=proc.wait();assert rc==0
 # Validate the completed temporary file before replacing the deliverable.
 check=cv2.VideoCapture(str(temporary));decoded=0
 assert int(check.get(cv2.CAP_PROP_FRAME_WIDTH))==W and int(check.get(cv2.CAP_PROP_FRAME_HEIGHT))==H
 while True:
  ok,frame=check.read()
  if not ok:break
  decoded+=1
 check.release();assert decoded==total, ('Incomplete final encode',decoded,total)
 temporary.replace(dest)
 (OUT/'edit-provenance.json').write_text(json.dumps({'source':'actual final composed game eyes','view':'left eye','fps':30,'duration':total/FPS,'music':'original procedural score','clips':clips},indent=2))
 print(dest,round(dest.stat().st_size/1048576,1),'MiB')
if __name__=='__main__':main()
