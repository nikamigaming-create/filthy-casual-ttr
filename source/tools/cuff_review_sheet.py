"""Arrange unaltered Blender renders for design review (no synthesized imagery)."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]/'assets'/'wrist-controls'
sheet=Image.new('RGB',(1800,1400),(35,37,37));draw=ImageDraw.Draw(sheet)
font=ImageFont.truetype('C:/Windows/Fonts/segoeuib.ttf',29)
small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',19)
for i,(file,label,sub) in enumerate([
 ('cuff-top.png','TOP','Recessed selectors and separate reload/settings buttons'),
 ('cuff-side.png','SIDE','Curved housing and fitted cuff around a neutral wrist'),
 ('cuff-worn.png','WORN','Actual 7 x 5 cm proposal on an original forearm proxy'),
 ('cuff-pressed.png','PRESSED','Weapon plunger depressed 1.6 mm; fixed rim stays put')]):
 x=(i%2)*900;y=(i//2)*700
 im=Image.open(root/file).convert('RGB');im.thumbnail((880,610))
 sheet.paste(im,(x+(900-im.width)//2,y+78))
 draw.text((x+22,y+12),label,font=font,fill=(229,226,208))
 draw.text((x+22,y+49),sub,font=small,fill=(184,188,182))
sheet.save(root/'design-review.png')
print(root/'design-review.png')
