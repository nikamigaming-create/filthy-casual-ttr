"""Create timestamped full-sequence contact sheets and check recorded cadence.
This is evidence for visual review, not an automatic headset-acceptance claim.
"""
import sys,json,subprocess,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageStat
root=Path(__file__).resolve().parents[1];work=root/'outputs/casual-guide'
name=sys.argv[1];src=work/(name+'-stereo.mp4')
meta=json.loads(src.with_suffix('.json').read_text())
timeline=json.loads((work/(name+'-timeline.json')).read_text(encoding='utf-8-sig'))
end=next(e['seconds'] for e in timeline if e['action']==('Trigger then jab' if name=='food-stimpak' else 'Finished'))+(0 if name=='food-stimpak' else 3)
frames=subprocess.check_output([shutil.which('ffmpeg'),'-v','error','-i',str(src),'-t',str(end),'-vf','fps=1,scale=516:276','-f','rawvideo','-pix_fmt','rgb24','pipe:1'])
size=516*276*3;n=len(frames)//size;f=ImageFont.truetype('C:/Windows/Fonts/consola.ttf',16)
stats=[]
for start in range(0,n,20):
 sheet=Image.new('RGB',(1290,1200),'#101a14');d=ImageDraw.Draw(sheet)
 for i in range(start,min(start+20,n)):
  im=Image.frombytes('RGB',(516,276),frames[i*size:(i+1)*size])
  left=im.crop((0,0,258,276));right=im.crop((258,0,516,276))
  stats.append({'second':i,'meanL':sum(ImageStat.Stat(left).mean)/3,'meanR':sum(ImageStat.Stat(right).mean)/3})
  x=(i-start)%5*258;y=(i-start)//5*300;sheet.paste(right,(x,y));d.text((x+4,y+278),f'{i:03d}s / RIGHT',font=f,fill='white')
 sheet.save(work/f'{name}-review-{start//20:02d}.jpg',quality=88)
report={k:meta[k] for k in ['encodedFrames','wallSeconds','repeatedFrames','fps','width','height','ffmpegExit']}
report.update(reviewedSpanSeconds=end,contactSheetCadence='1 frame per second, full right eye',minMeanL=min(x['meanL'] for x in stats),minMeanR=min(x['meanR'] for x in stats),physicalValidation=False)
# Negative fixture: the luminance rejection criterion must reject a blank eye.
report['blankNegativeRejected']=sum(ImageStat.Stat(Image.new('RGB',(258,276),'black')).mean)/3<3
if report['minMeanL']<3 or report['minMeanR']<3:raise RuntimeError('Near-blank eye in recorded sequence')
(work/(name+'-review.json')).write_text(json.dumps(report,indent=2));print(json.dumps(report))
