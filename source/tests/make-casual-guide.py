"""Assemble the captioned Filthy Casual TTR guide from actual final-eye captures.
No generated gameplay, frame interpolation, or speed changes. Original comic
art is a title/end graphic only. Run after reviewing captures and timing.
"""
from pathlib import Path
import json, math, subprocess, shutil
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'outputs/casual-guide'
WORK.mkdir(parents=True,exist_ok=True)
OUT=ROOT/'outputs/Filthy-Casual-TTR-Guide.mp4'
INK='#173b2c'; PAPER='#efdfb4'; CREAM='#fff0ce'; AMBER='#d37b24'
FF=shutil.which('ffmpeg'); FP=shutil.which('ffprobe')
def font(size,bold=False):
    return ImageFont.truetype('C:/Windows/Fonts/'+('arialbd.ttf' if bold else 'consola.ttf'),size)
def lines(draw,text,f,width):
    result=[]
    for para in text.split('\n'):
        line=''
        for word in para.split():
            trial=(line+' '+word).strip()
            if line and draw.textlength(trial,font=f)>width:result.append(line);line=word
            else:line=trial
        result.append(line)
    return result
def block(d,text,x,y,size,width,color=INK,bold=False,gap=12):
    f=font(size,bold)
    for line in lines(d,text,f,width):d.text((x,y),line,font=f,fill=color);y+=size+gap
    return y
def star(d,cx,cy,rx,ry,text):
    points=[]
    for i in range(32):
        a=math.pi*2*i/32;r=1 if i%2==0 else .86
        points.append((cx+math.cos(a)*rx*r,cy+math.sin(a)*ry*r))
    d.polygon(points,fill=AMBER,outline=INK,width=4)
    f=font(27,True);w=d.textlength(text,font=f);d.text((cx-w/2,cy-18),text,font=f,fill=CREAM)
def panel(name,title,body,control,chapter):
    im=Image.new('RGB',(1920,1080),PAPER);d=ImageDraw.Draw(im)
    for y in range(12,1080,18):
        for x in range(12+(y//18%2)*9,920,18):d.ellipse((x,y,x+2,y+2),fill='#c8ba91')
    d.rectangle((22,22,1898,1058),outline=INK,width=5)
    d.rectangle((945,48,1878,1035),fill=INK)
    d.text((64,53),'FILTHY CASUAL TTR',font=font(40,True),fill=INK)
    d.text((64,113),chapter.upper(),font=font(24),fill=INK)
    star(d,805,124,73,47,'PRO TIP')
    # Readable comic balloon stays outside the gameplay viewport.
    d.rounded_rectangle((55,187,885,658),radius=60,fill=CREAM,outline=INK,width=5)
    d.polygon([(685,653),(745,720),(778,653)],fill=CREAM)
    d.line([(685,653),(745,720),(778,653)],fill=INK,width=5)
    bottom=block(d,title,96,230,57,745,bold=True,gap=8)
    bottom=block(d,body,96,bottom+30,31,735,gap=12)
    if bottom>630:raise ValueError('Caption overflow: '+title)
    d.rounded_rectangle((60,770,880,917),radius=14,fill=INK)
    bottom=block(d,control,90,798,32,758,color=CREAM,bold=True,gap=10)
    if bottom>910:raise ValueError('Control overflow')
    d.text((64,976),'ACTUAL GAME / SIMULATED CONTROLLERS',font=font(24),fill=INK)
    d.text((64,1011),'FULL RIGHT EYE  |  SILENT COMIC GUIDE',font=font(22),fill=INK)
    im.save(WORK/(name+'.png'))
    return WORK/(name+'.png')

CAPTIONS={
'Seated before':('COUCH CLAIMED.','Sitting low in the wasteland?\nLook down. Let\'s get comfortable.','Stay seated. The height fix comes first.','01 / A chair is a tactical advantage'),
'Height controls':('NO NEED TO STAND.','Hold both sticks.\nRelease.','BOTH STICKS: hold 0.6 seconds','01 / Your couch has clearance'),
'Calibrate':('AHH. MUCH BETTER.','Stay seated.\nWatch your viewpoint rise.','A: calibrate to 165 cm','01 / The view rises. You relax.'),
'Save height':('SAVE YOUR SEAT.','Press B to save and close.\nFine-tune later with right stick up/down.','B: save  |  Right stick: +/- 5 cm','01 / Ready for the wasteland'),
'Open weapons':('NO MORE RUMMAGING.','Click the right stick and let go.\nThe weapon wheel stays put in the world.','RIGHT STICK CLICK: weapons','02 / A gentleman chooses his hardware'),
'Touch pistol':('TOUCH. EQUIP. RELAX.','Touch a preview. No grabbing needed.\nThe normal-size gun goes to your right hand.','Either hand can select.','02 / Small previews. Real-size hardware.'),
'Shoot and auto reload':('RELOAD? HOW QUAINT.','Fire with the right trigger.\nWhen the magazine empties, it reloads automatically.','RIGHT TRIGGER: fire','02 / Less fiddling. More pew.'),
'A reload':('TOP IT OFF.','Reload before empty.','A: reload with the wheel closed','02 / No bolt gymnastics required'),
'Center cap':('HANDS FREE, PAL.','Open the wheel and touch the big bottle cap.\nYour selection clears.','CENTER CAP: select nothing','03 / A proper neutral option'),
'Run':('THE COUCH CAN RUN.','Push the left stick fully to run.\nDo not click it: that opens consumables.','LEFT STICK: move / run','03 / Your legs can stay on vacation'),
'Open food':('SNACKS WITHIN REACH.','Click the left stick. Touch your food.\nIt goes to your right hand without being used.','LEFT STICK CLICK: consumables','04 / Emergency snack deployment'),
'Menu retention':('KEEP YOUR SNACK.','This is the native pause screen.\nReturn to the same food still in your hand.','MENU TRANSITION: no accidental drop','04 / No floor seasoning, thanks'),
'Y consume':('Y = SNACK TIME.','Press Y to use the food in your hand.\nOne item. Its normal game effects.','Y: consume selected ordinary food / aid','04 / The right to remain well fed'),
'Open Stimpak':('A LITTLE PICK-ME-UP.','Choose a Stimpak.\nThis is TTWVR\'s native live Stimpak.','Selection alone does not use a dose.','05 / The original VR action'),
'Trigger then jab':('TRIGGER. THEN JAB.','Hold the RIGHT trigger, then jab into your body.\nA jab alone is not the activation step.','HOLD RIGHT TRIGGER + BODY JAB','05 / One native Stimpak dose'),
'Quiet height panel':('LESS CHATTER.','Selector popups default to off.\nY in this panel toggles them. Height messages stay on.','HEIGHT PANEL: Y toggles hints; B closes','06 / Peace, quiet, and a comfy chair'),
}

def run(args):subprocess.run(args,check=True,timeout=300)
def probe(p):return json.loads(subprocess.check_output([FP,'-v','error','-show_format','-show_streams','-of','json',str(p)],text=True))
ENC=['-c:v','libx264','-preset','veryfast','-crf','18','-pix_fmt','yuv420p','-threads','4','-an']
BASE=[FF,'-hide_banner','-loglevel','error','-y']
files=[];segments=[]

# The illustration is used unchanged as a scaled video background. Title text
# and comic lettering are a separate transparent graphic layered by ffmpeg.
title=Image.new('RGBA',(1920,1080),(0,0,0,0));d=ImageDraw.Draw(title)
block(d,'FILTHY\nCASUAL\nTTR',72,145,106,700,color=CREAM,bold=True,gap=2)
block(d,'WE LOVE A TALE OF TWO REALITIES.\nWE\'RE JUST FILTHY CASUALS.',78,535,28,700,color=CREAM,bold=True,gap=13)
block(d,'Can\'t be bothered to reload?\nTired of digging for snacks?\nStay seated. We\'ve got you.',78,700,29,630,color=CREAM,gap=14)
d.text((78,970),'A COUCH SURVIVAL GUIDE / v1.2.2',font=font(24),fill=CREAM)
block(d,'STAY\nSEATED!',890,103,37,245,bold=True,gap=1)
block(d,'COMFORT\nIS KING.',1710,475,24,175,bold=True,gap=2)
block(d,'LESS\nFUSS!',809,424,31,180,bold=True,gap=2)
title.save(WORK/'title-type.png')
hero=ROOT/'docs/assets/filthy-casual-recliner.png'
titleout=WORK/'00-title.mp4'
run(BASE+['-loop','1','-framerate','30','-i',str(hero),'-loop','1','-framerate','30','-i',str(WORK/'title-type.png'),'-filter_complex','[0:v]scale=1920:1080,setsar=1[b];[b][1:v]overlay=0:0,format=yuv420p[v]','-map','[v]','-t','14']+ENC+[str(titleout)])
files.append(titleout)

index=1
for take in ['height-weapons','food-stimpak','stimpak']:
    source=WORK/(take+'-stereo.mp4');data=probe(source)
    stream=next(s for s in data['streams'] if s['codec_type']=='video')
    if (int(stream['width']),int(stream['height']))!=(2064,1104):raise ValueError('Wrong eye capture dimensions')
    timeline=json.loads((WORK/(take+'-timeline.json')).read_text(encoding='utf-8-sig'))
    finished=[e['seconds'] for e in timeline if e['action']=='Finished']
    finish=min(float(data['format']['duration']),finished[0]+3 if finished else float(data['format']['duration']))
    if take=='height-weapons':finish=min(finish,next(e['seconds'] for e in timeline if e['action']=='Run'))
    # The original food take's jab missed. Preserve that source, but replace
    # its jab chapter with the separately verified seated native action.
    if take=='food-stimpak':finish=min(finish,next(e['seconds'] for e in timeline if e['action']=='Trigger then jab'))
    events=[e for e in timeline if e['action'] in CAPTIONS and e['seconds']<finish]
    for i,event in enumerate(events):
        start=0 if i==0 else event['seconds'];end=events[i+1]['seconds'] if i+1<len(events) else finish
        action=event['action'];name=f'{index:02d}-{take}'
        bg=panel(name,*CAPTIONS[action]);target=WORK/(name+'.mp4')
        filters=('[1:v]crop=1032:1104:1032:0,scale=910:974,setsar=1,setpts=PTS-STARTPTS,'
                 'eq=gamma=1.10:brightness=0.015[eye];[0:v][eye]overlay=957:55:shortest=1,format=yuv420p[v]')
        run(BASE+['-loop','1','-framerate','30','-i',str(bg),'-ss',str(start),'-t',str(end-start),'-i',str(source),'-filter_complex',filters,'-map','[v]','-t',str(end-start)]+ENC+[str(target)])
        files.append(target);segments.append(dict(source=str(source),start=start,end=end,action=action));index+=1

# Installation is a truthful, labelled screenshot guide from the normal profile.
for key,title,body,control,screenshot in [
    ('install','ONE LITTLE ZIP.','In MO2, install Filthy-Casual-TTR-1.2.2.zip.\nUse a working TTR / TTWVR setup.','Install a new mod from an archive.','mo2-installed.png'),
    ('enable','TICK. LAUNCH. LOUNGE.','Enable Filthy Casual TTR below the base mod.\nStart your usual TTWVR entry through MO2.','Keep only one version of this addon enabled.','mo2-installed.png'),
]:
    bg=panel('card-'+key,title,body,control,'07 / Installation through MO2')
    target=WORK/(key+'.mp4')
    filters='[1:v]scale=900:900:force_original_aspect_ratio=decrease,pad=910:974:(ow-iw)/2:(oh-ih)/2:color=0x173b2c,setsar=1[shot];[0:v][shot]overlay=957:55,format=yuv420p[v]'
    # These are screenshots, not live gameplay. Label the image explicitly.
    im=Image.open(bg);dd=ImageDraw.Draw(im);dd.rectangle((52,960,920,1040),fill=PAPER);dd.text((64,976),'ACTUAL MO2 / FILTERED TO THIS ADDON',font=font(23),fill=INK);dd.text((64,1011),'Your base game and other mods stay enabled.',font=font(22),fill=INK);im.save(bg)
    run(BASE+['-loop','1','-framerate','30','-i',str(bg),'-loop','1','-framerate','30','-i',str(WORK/screenshot),'-filter_complex',filters,'-map','[v]','-t','10']+ENC+[str(target)])
    files.append(target)

endcard=panel('thanks','THANKS, KUPOKINZ.','A Tale of Two Realities makes the VR possible.\nFilthy Casual TTR adds our couch conveniences.','To run: push the left stick fully. No click.','The wasteland is better together')
im=Image.open(endcard);dd=ImageDraw.Draw(im);dd.rectangle((52,960,920,1040),fill=PAPER);dd.text((64,976),'ORIGINAL ILLUSTRATION / COMMUNITY ADDON',font=font(23),fill=INK);dd.text((64,1011),'Built on KUPOkinz\'s A Tale of Two Realities.',font=font(22),fill=INK);im.save(endcard)
target=WORK/'thanks.mp4'
run(BASE+['-loop','1','-framerate','30','-i',str(endcard),'-stream_loop','-1','-i',str(titleout),'-filter_complex','[1:v]scale=900:506,setsar=1[h];[0:v][h]overlay=962:270,format=yuv420p[v]','-map','[v]','-t','9']+ENC+[str(target)])
files.append(target)
concat=WORK/'concat.txt';concat.write_text(''.join("file '"+p.name+"'\n" for p in files))
run(BASE+['-f','concat','-safe','0','-i',str(concat),'-c','copy','-movflags','+faststart',str(OUT)])
info=probe(OUT)
(WORK/'assembly.json').write_text(json.dumps(dict(output=str(OUT),duration=float(info['format']['duration']),silent=True,simulatedControllers=True,fullRightEye=True,frameInterpolation=False,speedChanges=False,videoGrade='gamma 1.10, brightness +0.015; original stereo retained',segments=segments),indent=2))
print(OUT)
