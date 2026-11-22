"""Current v1.4.4 showcase. Actual final-eye footage at real time; original graphic overlays."""
from pathlib import Path
import json, subprocess, math, shutil, sys
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(r'D:\code\ttwvr-seated-controls')
WORK=ROOT/'outputs/human-showcase/edit'
WORK.mkdir(parents=True,exist_ok=True)
SRC=WORK.parent
OUT=ROOT/'outputs/Filthy-Casual-TTR-Showcase.mp4'
INK='#183c31'; PAPER='#efdfb4'; CREAM='#fff1d1'; RED='#a63828'; GOLD='#e7a33a'
FF=shutil.which('ffmpeg')
BASE=[FF,'-hide_banner','-loglevel','error','-y']
ENC=['-c:v','libx264','-preset','veryfast','-crf','18','-pix_fmt','yuv420p','-threads','4','-an']
def run(args): subprocess.run(args,check=True,timeout=600)
def font(n,bold=False): return ImageFont.truetype('C:/Windows/Fonts/'+('arialbd.ttf' if bold else 'consola.ttf'),n)
def block(d,txt,x,y,n,width,color=INK,bold=False,gap=12):
    f=font(n,bold)
    for para in txt.split('\n'):
        line=''
        for word in para.split():
            cand=(line+' '+word).strip()
            if line and d.textlength(cand,font=f)>width:
                d.text((x,y),line,font=f,fill=color);y+=n+gap;line=word
            else:line=cand
        d.text((x,y),line,font=f,fill=color);y+=n+gap
    return y
def paper():
    im=Image.new('RGB',(1920,1080),PAPER);d=ImageDraw.Draw(im)
    for y in range(0,1080,16):
        for x in range((y//16%2)*8,1920,16):d.ellipse((x,y,x+2,y+2),fill='#cdbd96')
    d.rectangle((20,20,1900,1060),outline=INK,width=6)
    return im,d
def burst(d,cx,cy,rx,ry,text):
    pts=[(cx+math.cos(i*math.pi/16)*rx*(1 if i%2==0 else .84),cy+math.sin(i*math.pi/16)*ry*(1 if i%2==0 else .84)) for i in range(32)]
    d.polygon(pts,fill=GOLD,outline=INK,width=4)
    f=font(28,True);d.text((cx-d.textlength(text,font=f)/2,cy-18),text,font=f,fill=INK)

im,d=paper()
d.rectangle((922,43,1885,1040),fill=INK)
d.text((65,55),'NIKAMI / COUCH SURVIVAL',font=font(29,True),fill=INK)
block(d,'FILTHY\nCASUAL TTR',65,112,66,810,bold=True,gap=0)
d.text((67,271),'v1.4.4',font=font(27),fill=RED)
d.text((65,985),'YOUR INVENTORY. WITHIN REACH.',font=font(27,True),fill=INK)
im.save(WORK/'base.png')

def bubble(name,title,body,control,stamp):
    im=Image.new('RGBA',(870,620));d=ImageDraw.Draw(im)
    d.rounded_rectangle((7,8,845,358),radius=45,fill=CREAM,outline=INK,width=5)
    d.polygon([(595,354),(651,411),(697,354)],fill=CREAM)
    d.line([(595,354),(651,411),(697,354)],fill=INK,width=5)
    y=block(d,title,45,44,53,755,bold=True,gap=6)
    y=block(d,body,47,y+23,31,750,gap=12)
    if y>345:raise ValueError((name,y))
    d.rounded_rectangle((10,448,845,604),radius=15,fill=INK)
    y=block(d,control,38,469,28,770,color=CREAM,bold=True,gap=9)
    if y>598:raise ValueError((name,'controls overflow',y))
    burst(d,748,407,94,46,stamp)
    p=WORK/(name+'.png');im.save(p);return p

C1=[
(0,'SITTING LOW?','A chair is a tactical advantage.\nFirst, look down and set your height.','Stay seated. Your viewpoint does the work.','COZY!'),
(7.1,'YOUR SEAT. YOUR HEIGHT.','Hold the LEFT Menu button.\nThe height controls appear.','LEFT MENU: hold 1 second','HOLD!'),
(10.9,'AHH. MUCH BETTER.','A calibrates. Right stick fine-tunes.\nB saves and closes.','A: calibrate  /  B: save','UP!'),
(15.4,'SET IT ONCE.','Your saved seated height returns\nnext time you launch and load.','B saves your height for future sessions.','SAVED!'),
(20.4,'PICK YOUR HARDWARE.','Compact previews stay out in the world.\nTake a moment. Choose your weapon.','Touch LEFT stick + double-tap RIGHT thumbrest','TAP TAP'),
(27,'TOUCH. EQUIP. RELAX.','Touch a preview. The normal-size\nweapon goes into your right hand.','No grab required. Selection closes the wheel.','READY!'),
(36.85,'LESS FUSS. MORE PEW.','Fire normally. Automatic mode reloads\nwhen the magazine runs empty.','RIGHT TRIGGER: native firing','PEW!'),
(43.4,'HANDS-ON? YOUR CALL.','Tap Menu for manual reload mode.\nYour native ammo supply appears below.','LEFT MENU: tap to change reload mode','CHOICE!'),
(51,'BACK TO EASY STREET.','Another tap returns to automatic mode.\nChoose how much work you want to do.','Manual or automatic. Switch during play.','EASY!'),
(54.96,'HANDS FREE, PAL.','Open the wheel. Touch its center cap.\nThe weapon leaves your hand.','CENTER CAP: clear selection','CLEAR!')]
C2=[
(0,'SNACKS WITHIN REACH.','Your aid inventory has its own wheel.\nThe same touch-to-select idea.','Touch RIGHT stick + double-tap LEFT thumbrest','TAP TAP'),
(6.5,'SELECT. DON\'T SPEND.','The item goes to your native right hand.\nSelecting it does not consume it.','Native item interactions take over.','HELD!'),
(17.14,'DONE WITH THAT?','Touch the center cap to clear the hand.\nOr squeeze right grip for native holding.','CENTER CAP: clear hand  /  RIGHT GRIP: native mode','CLEAR!'),
(31.59,'A LITTLE PERSONALITY.','Relax. Curl. Open your hands again.\nLet each motion breathe.','Quest controller-driven hand poses','HELLO!'),
(37.64,'MAKE YOUR POINT.','Index touch and grip drive the pose.\nA slow turn makes the gesture readable.','Thumb + index sensors; grouped grip fingers','LOOK!'),
(42.91,'COUCH APPROVED.','Lift your thumb off the touch surfaces\nwhile keeping a deliberate grip.','Lift your thumb. Keep your grip held.','NICE!'),
(47.3,'SEE YOU OUT THERE.','A relaxed wave. Then back to the game.\nNo rush. The couch is not going anywhere.','Stay seated. Stay comfortable.','RELAX!')]

def take(name,captions,duration):
    cached=WORK/(name+'-edit.mp4')
    if cached.exists() and ('--reuse-takes' in sys.argv or ('--reuse-unaffected' in sys.argv and name!='01-height-weapons')): return cached
    ins=['-loop','1','-framerate','30','-i',str(WORK/'base.png'),'-i',str(SRC/(name+'-stereo.mp4'))]
    filters=['[1:v]crop=1032:1104:1032:0,scale=920:984,eq=gamma=1.25,setsar=1[g]','[0:v][g]overlay=944:52:shortest=1[b0]']
    for i,(t,title,body,control,stamp) in enumerate(captions):
        p=bubble(name+f'-{i}',title,body,control,stamp)
        ins+=['-loop','1','-framerate','30','-i',str(p)]
        end=captions[i+1][0] if i+1<len(captions) else duration
        # Brief comic slide-up on each new bubble. Gameplay stays continuous.
        filters.append(f"[b{i}][{i+2}:v]overlay=x=47:y='330+24*exp(-18*max(0,t-{t}))':enable='gte(t,{t})*lt(t,{end})'[b{i+1}]")
    out=WORK/(name+'-edit.mp4')
    run(BASE+ins+['-filter_complex_threads','2','-filter_complex',';'.join(filters),'-map',f'[b{len(captions)}]','-t',str(duration),'-r','30']+ENC+[str(out)])
    print('Rendered',out,flush=True);return out

def static(name,im,duration):
    p=WORK/(name+'.png');im.save(p);v=WORK/(name+'.mp4')
    run(BASE+['-loop','1','-framerate','30','-i',str(p),'-t',str(duration),'-vf','setsar=1']+ENC+[str(v)])
    return v

# The existing original title illustration is unchanged, scaled behind type.
title=Image.new('RGBA',(1920,1080));d=ImageDraw.Draw(title)
block(d,'STAY\nSEATED!',905,111,35,245,bold=True,gap=0)
block(d,'LESS\nFUSS!',846,462,27,180,bold=True,gap=1)
block(d,'COMFORT\nIS KING.',1710,492,23,178,bold=True,gap=1)
d.rounded_rectangle((42,48,835,1030),radius=36,fill=(19,51,41,238),outline=CREAM,width=4)
block(d,'FILTHY\nCASUAL\nTTR',83,100,110,710,color=CREAM,bold=True,gap=0)
block(d,'FOR THE COUCH COMMANDER.',88,470,31,700,color=GOLD,bold=True)
block(d,"Can't be bothered to reload?\nTired of digging for a consumable?\nStay seated. Let's make it easy.",88,555,34,695,color=CREAM,gap=18)
block(d,"We love A Tale of Two Realities.\nWe're just filthy casuals.",88,800,29,690,color=CREAM,gap=13)
d.text((88,965),'NIKAMI  /  v1.4.4',font=font(26),fill=GOLD)
title.save(WORK/'title-overlay.png')
titleout=WORK/'00-title.mp4'
run(BASE+['-loop','1','-framerate','30','-i',str(ROOT/'docs/assets/filthy-casual-recliner.png'),'-loop','1','-framerate','30','-i',str(WORK/'title-overlay.png'),'-filter_complex','[0:v]scale=1920:1080,setsar=1[b];[b][1:v]overlay=0:0[v]','-map','[v]','-t','9']+ENC+[str(titleout)])
C3=[
(0,'A LITTLE PERSONALITY.','Relax. Curl. Open your hands again.\nLet each motion breathe.','Quest controller-driven hand poses','HELLO!'),
(7.59,'MAKE YOUR POINT.','Index touch and grip drive the pose.\nA slow turn makes the gesture readable.','Thumb + index sensors; grouped grip fingers','LOOK!'),
(15.16,'COUCH APPROVED.','Lift your thumb off the touch surfaces\nwhile keeping a deliberate grip.','Lift your thumb. Keep your grip held.','NICE!'),
(20.60,'SEE YOU OUT THERE.','A relaxed wave. Then back to the game.\nNo rush. The couch is not going anywhere.','Stay seated. Stay comfortable.','RELAX!')]
clips=[titleout,take('01-height-weapons',C1,72),take('02-aid-gestures',C2[:3],31.5),take('03-gestures',C3,31)]

im,d=paper();block(d,'TWO MODS. YOUR CHOICE.',75,70,77,1760,bold=True)
block(d,'FILTHY CASUAL TTR',80,235,53,830,bold=True)
block(d,'Seated height saved between sessions\nFloating inventory selectors\nReload mode + hand gestures',85,330,38,810,gap=26)
block(d,'TTR META LINK',1030,235,53,800,bold=True)
block(d,'Quest -> Meta Link / Air Link\n-> Meta OpenXR -> TTR\nSteamVR / VD not required',1035,330,36,790,gap=26)
d.line((960,225,960,705),fill=INK,width=4)
block(d,'Choose your comfort controls. Choose your connection.',80,750,40,1750,bold=True)
block(d,'Install either addon, or use them together.',80,870,33,1750)
clips.append(static('03-two-mods',im,9))

im,d=paper();block(d,'INSTALL. ENABLE. GET COMFY.',75,75,73,1760,bold=True)
steps=[('01','START WITH WORKING TTR','Check each README for the exact supported base build.'),('02','ADD THE ZIP TO MOD ORGANIZER 2','Use Install a new mod from an archive. Choose either or both ZIPs.'),('03','ENABLE BELOW TTWVR','Connect your headset. Run your usual game entry through MO2.')]
for i,(n,heading,body) in enumerate(steps):
    y=255+i*207;d.ellipse((80,y,195,y+115),fill=RED);d.text((104,y+29),n,font=font(44,True),fill=CREAM)
    block(d,heading,235,y+4,43,1560,bold=True);block(d,body,238,y+77,30,1540)
block(d,'Keep your usual TTR setup. Add a little couch comfort.',80,935,32,1750)
clips.append(static('04-install',im,13))

im,d=paper();block(d,'YOUR COUCH IS CLEARED\nFOR THE WASTELAND.',75,80,78,1740,bold=True,gap=8)
block(d,'SOURCE + MO2 DOWNLOADS',80,365,38,1700,color=RED,bold=True)
block(d,'github.com/nikamigaming-create/filthy-casual-ttr\ngithub.com/nikamigaming-create/ttr-meta-link',80,445,37,1740,gap=26)
block(d,'Base VR by KUPOkinz.\nWe love A Tale of Two Realities. We\'re just filthy casuals.',80,720,32,1730,gap=17)
block(d,'STAY SEATED. SEE YOU IN THE WASTELAND.',80,966,30,1730,bold=True)
clips.append(static('05-end',im,10))

# Half-second comic wipes only between complete chapters; no action is sped up.
durations=[9,72,31.5,31,9,13,10]
ins=[]
for c in clips:ins+=['-i',str(c)]
filters=[];offset=durations[0]-.5;last='0:v'
for i in range(1,len(clips)):
    target=f'x{i}';transition='wipeleft' if i%2 else 'smoothright'
    filters.append(f'[{last}][{i}:v]xfade=transition={transition}:duration=0.5:offset={offset:.3f}[{target}]')
    offset+=durations[i]-.5;last=target
run(BASE+ins+['-filter_complex_threads','2','-filter_complex',';'.join(filters),'-map',f'[{last}]','-movflags','+faststart']+ENC+[str(OUT)])
(WORK/'edit-manifest.json').write_text(json.dumps({'output':str(OUT),'clips':[str(c) for c in clips],'sourceDurations':durations,'duration':sum(durations)-.5*(len(clips)-1),'footageSpeed':1,'eye':'complete right eye','grade':'global gamma 1.25','input':'choreographed private XR simulator','audio':False,'frameInterpolation':False},indent=2))
print('FINAL',OUT,flush=True)
