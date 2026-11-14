"""Build the local silent guide. No runtime/game control or network operations.

Prepare cards: python tests/make-guide.py --prepare-only
Render only after the capture is reviewed: python tests/make-guide.py --capture-approved
"""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import shutil
import subprocess
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'outputs' / 'guide'
FINAL = ROOT / 'outputs' / 'TTWVR-Seated-Controls-Guide.mp4'
BG = '#09140e'
GREEN = '#a7eab4'
AMBER = '#edbe70'
WHITE = '#d4dfd7'
MUTED = '#8fa997'
FONTROOT = Path('C:/Windows/Fonts')

def font(size, bold=False):
    for name in (['consolab.ttf', 'arialbd.ttf'] if bold else ['consola.ttf', 'arial.ttf']):
        p = FONTROOT / name
        if p.exists():
            return ImageFont.truetype(str(p), size)
    return ImageFont.load_default()

def wrap(draw, value, face, width):
    lines=[]
    for paragraph in value.split('\n'):
        words=paragraph.split()
        line=''
        for word in words:
            candidate=(line+' '+word).strip()
            if line and draw.textlength(candidate,font=face)>width:
                lines.append(line); line=word
            else:
                line=candidate
        lines.append(line)
    return lines

def textblock(draw, value, xy, size=32, fill=WHITE, width=770, bold=False, gap=14):
    face=font(size,bold)
    x,y=xy
    for line in wrap(draw,value,face,width):
        draw.text((x,y),line,font=face,fill=fill)
        y+=size+gap
    return y

def card(title, body, chapter, footer, screenshot=None, focus=None):
    im=Image.new('RGB',(1920,1080),BG)
    d=ImageDraw.Draw(im)
    for x in range(0,1920,48): d.line((x,0,x,1080),fill='#10251a')
    for y in range(0,1080,48): d.line((0,y,1920,y),fill='#10251a')
    d.rounded_rectangle((24,24,1896,1056),radius=15,outline='#426b4d',width=2)
    d.line((925,100,925,998),fill='#426b4d',width=2)
    d.text((66,54),'TTWVR  /  SEATED CONTROLS',font=font(28,True),fill=GREEN)
    d.text((66,132),chapter.upper(),font=font(25),fill=AMBER)
    bottom=textblock(d,title,(62,192),size=55,fill=GREEN,bold=True,gap=10)
    d.line((66,bottom+20,850,bottom+20),fill='#426b4d',width=2)
    bodybottom=textblock(d,body,(66,bottom+57),size=31,gap=17)
    if bodybottom>930:
        raise ValueError('Caption is too long for the instruction panel: '+title)
    d.text((66,1005),'SILENT GUIDE  /  FOLLOW THE CAPTIONS',font=font(22),fill=MUTED)
    d.text((970,1005 if screenshot else 38),footer,font=font(20),fill=MUTED)
    if screenshot:
        source=Image.open(WORK/screenshot).convert('RGB')
        # Preserve the entire actual application screenshot. Focus inset is
        # separately labelled; it is never presented as a live interaction.
        source.thumbnail((900,766),Image.Resampling.LANCZOS)
        x=970+(900-source.width)//2
        y=140+(766-source.height)//2
        im.paste(source,(x,y))
        d.rectangle((x-2,y-2,x+source.width+2,y+source.height+2),outline=AMBER,width=2)
        d.text((970,91),'ACTUAL MO2 SCREENSHOT',font=font(23,True),fill=AMBER)
        if focus:
            original=Image.open(WORK/screenshot).convert('RGB')
            crop=original.crop(focus)
            factor=min(780/crop.width,180/crop.height)
            crop=crop.resize((round(crop.width*factor),round(crop.height*factor)),Image.Resampling.LANCZOS)
            fy=max(bodybottom+35,720)
            if fy+crop.height<966:
                d.text((66,fy-31),'DETAIL FROM SCREENSHOT',font=font(20),fill=AMBER)
                im.paste(crop,(66,fy))
                d.rectangle((64,fy-2,68+crop.width,fy+crop.height+2),outline=AMBER,width=2)
    else:
        # Full right-eye source will cover this area, with no additional crop.
        d.rectangle((968,68,1872,1034),outline='#426b4d',width=2)
    return im

INSTALL = [
    ('01-install.png','Start with working TTWVR.',
     'Use your usual TTWVR profile.\n\nInstall a new mod from an archive. Choose the Seated Controls ZIP.\n\nKeep the base mod enabled.',
     '01 / Install the addon','mo2-before.png',(0,40,120,100)),
    ('02-install.png','Install the ZIP.',
     'Name it TTWVR - Seated Controls.\n\nConfirm Quick Install.\n\nThe ZIP already has the correct data root.',
     '02 / Confirm installation','mo2-install.png',(440,490,840,605)),
    ('03-install.png','Enable. Then launch.',
     'Enable the addon below the base TTWVR mod.\n\nLaunch your normal TTWVR executable through MO2.\n\nThe OpenXR layer loads automatically.',
     '03 / Play from MO2','mo2-installed.png',(0,405,775,478)),
]
CHAPTERS={
    'weapon':('Reach for a weapon.',
       'Click and release the right stick.\n\nTouch a compact preview with either hand. The native weapon equips in your right hand.\n\nUse TTWVR\'s normal firing controls.',
       '04 / Weapons'),
    'aid':('Pick up native aid.',
       'Click and release the left stick. Touch an item preview.\n\nThe full-size item goes to your right hand.\n\nUse the native grip/release and backpack controls. Selection does not consume it.',
       '05 / Aid and stowing'),
    'stim':('Stimpaks stay native.',
       'Use A / X to change selector pages. Touch the Stimpak preview.\n\nTTWVR holds the live Stimpak in the right hand.\n\nThis section shows selection and put-away; no custom consume action is added.',
       '06 / Stimpak selection'),
    'height':('Set your seated height.',
       'Hold both sticks 0.6 s; release.\n\nA  Calibrate: 165 cm\nRight stick  +/- 5 cm\nX  Reset\nB  Save and close',
       '07 / Seated height'),
}

def prepare():
    WORK.mkdir(parents=True,exist_ok=True)
    for name,title,body,chapter,source,focus in INSTALL:
        card(title,body,chapter,'STEP-BY-STEP SCREENSHOT GUIDE',source,focus).save(WORK/name)
    for key,(title,body,chapter) in CHAPTERS.items():
        card(title,body,chapter,'IN-GAME CAPTURE / SIMULATED CONTROLLERS').save(WORK/('panel-'+key+'.png'))
    print('Prepared 3 installation cards and 4 gameplay panels.',flush=True)

def run(args):
    subprocess.run(args,check=True,timeout=300)

def probe(path):
    result=subprocess.check_output([shutil.which('ffprobe') or 'ffprobe','-v','error','-show_streams','-show_format','-of','json',str(path)],text=True)
    return json.loads(result)

def chapters(duration):
    timeline=json.loads((WORK/'timeline.json').read_text(encoding='utf-8-sig'))
    if isinstance(timeline,dict): timeline=timeline.get('segments',timeline.get('events',[]))
    events=[]
    for item in timeline:
        start=float(item.get('seconds',item.get('start',0)))
        action=str(item.get('action',item.get('title','')))
        events.append((start,action))
    events.sort()
    starts=[(0.0,'weapon')]
    finish=duration
    for seconds,action in events:
        lowered=action.lower()
        if 'open consumables' in lowered: starts.append((seconds,'aid'))
        elif 'page to stimpak' in lowered: starts.append((seconds,'stim'))
        elif 'seated height' in lowered: starts.append((seconds,'height'))
        elif lowered=='finished': finish=min(duration,math.ceil(seconds+1.0))
    if len(starts)!=4:
        raise ValueError('Timeline must contain Open consumables, Page to Stimpak, and Seated height.')
    starts.sort()
    result=[]
    for i,(start,key) in enumerate(starts):
        end=starts[i+1][0] if i+1<len(starts) else finish
        if end<=start: raise ValueError('Invalid chapter timing')
        result.append((start,end,key))
    return result

def render(weapon_replacement=False):
    source=WORK/'gameplay-stereo.mp4'
    data=probe(source)
    stream=next(s for s in data['streams'] if s['codec_type']=='video')
    width,height=int(stream['width']),int(stream['height'])
    if (width,height)!=(2064,1104): raise ValueError('Expected 2064 x 1104 stereo capture')
    duration=float(data['format']['duration'])
    segments=chapters(duration)
    jobs=[(source,start,end,key) for start,end,key in segments]
    if weapon_replacement:
        weapon=WORK/'weapon-stereo.mp4'
        weapondata=probe(weapon)
        wstream=next(s for s in weapondata['streams'] if s['codec_type']=='video')
        if (int(wstream['width']),int(wstream['height']))!=(2064,1104):
            raise ValueError('Weapon replacement must be actual 2064 x 1104 stereo')
        weaponend=float(weapondata['format']['duration'])
        events=json.loads((WORK/'weapon-timeline.json').read_text(encoding='utf-8-sig'))
        if isinstance(events,dict): events=events.get('events',events.get('segments',[]))
        for event in events:
            if str(event.get('action','')).lower()=='finished':
                weaponend=min(weaponend,math.ceil(float(event['seconds'])+1.0))
        jobs=[(weapon,0.0,weaponend,'weapon')]+[job for job in jobs if job[3]!='weapon']
    ffmpeg=shutil.which('ffmpeg') or 'ffmpeg'
    base=[ffmpeg,'-hide_banner','-loglevel','error','-y']
    encode=['-c:v','libx264','-preset','veryfast','-crf','19','-pix_fmt','yuv420p','-threads','4','-an']
    files=[]
    for i,(name,*_) in enumerate(INSTALL):
        target=WORK/f'guide-install-{i+1}.mp4'
        run(base+['-loop','1','-framerate','30','-i',str(WORK/name),'-t','6']+encode+[str(target)])
        files.append(target)
    for clipsource,start,end,key in jobs:
        # Trim actual frames, retain the complete right eye, preserve aspect.
        # 900x962 is the nearest even-sized fit for the 1032x1104 eye image.
        target=WORK/f'guide-game-{key}.mp4'
        filters=('[1:v]crop=1032:1104:1032:0,scale=900:962:force_original_aspect_ratio=decrease,pad=900:962:(ow-iw)/2:(oh-ih)/2,setsar=1,setpts=PTS-STARTPTS[eye];'
                 '[0:v][eye]overlay=970:70:shortest=1,format=yuv420p[v]')
        run(base+['-loop','1','-framerate','30','-i',str(WORK/('panel-'+key+'.png')),
             '-ss',f'{start:.6f}','-t',f'{end-start:.6f}','-i',str(clipsource),
             '-filter_complex',filters,'-map','[v]','-t',f'{end-start:.6f}']+encode+[str(target)])
        files.append(target)
    concat=WORK/'guide-concat.txt'
    concat.write_text(''.join("file '"+p.name+"'\n" for p in files),encoding='utf-8')
    pending=WORK/'guide-assembled.mp4'
    run(base+['-f','concat','-safe','0','-i',str(concat),'-c','copy','-movflags','+faststart',str(pending)])
    verified=probe(pending)
    v=next(s for s in verified['streams'] if s['codec_type']=='video')
    if (int(v['width']),int(v['height']))!=(1920,1080): raise ValueError('Wrong guide dimensions')
    shutil.copyfile(pending,FINAL)
    (WORK/'assembly.json').write_text(json.dumps({
        'output':str(FINAL),'silent':True,'installation':'three actual screenshots, 6 seconds each',
        'gameplay':'uninterpolated actual right-eye capture; full eye retained; simulated controllers',
        'source':str(source),'chapters':[{'source':str(p),'start':s,'end':e,'key':k} for p,s,e,k in jobs],
        'timelineClockNote':'action clock begins after recorder process start; approximately 0.2 seconds skew',
        'duration':float(verified['format']['duration']),'physicalHeadsetValidation':False
    },indent=2),encoding='utf-8')
    print(str(FINAL),flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare-only',action='store_true')
    parser.add_argument('--capture-approved',action='store_true')
    parser.add_argument('--weapon-replacement',action='store_true',help='Use reviewed weapon-stereo.mp4 and weapon-timeline.json for the weapon chapter.')
    args=parser.parse_args()
    prepare()
    if args.prepare_only:
        pass
    elif args.capture_approved:
        render(args.weapon_replacement)
    else:
        parser.error('Final render requires --capture-approved after capture review. Use --prepare-only for cards.')



