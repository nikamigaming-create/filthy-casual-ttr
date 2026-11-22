"""Compress a current reviewed-caption tutorial export for a Drive upload.
Run make-tutorial-video.py --render first. Never replace it with stale captions.
"""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess
R=Path(__file__).resolve().parents[1]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--render',action='store_true');ap.add_argument('--encoder',choices=['libx264','h264_nvenc'],default='libx264');a=ap.parse_args()
 manifest=json.loads((R/'work/tutorial-video/edit-manifest.json').read_text())
 source=Path(manifest['output'])
 if manifest.get('captionRevision',0)<2:raise SystemExit('Source predates reviewed captions; rebuild the full tutorial first. Existing phone copy is unchanged.')
 if hashlib.sha256(source.read_bytes()).hexdigest()!=manifest['outputSHA256']:raise SystemExit('Source hash differs from completed export')
 out=R/'outputs/Filthy-Casual-TTR-Reload-Guide-Phone.mp4'
 if not a.render:print('Validated current source; pass --render to encode the phone copy.');return
 tmp=out.with_name(out.stem+'-encoding.mp4')
 enc=['-c:v','h264_nvenc','-preset','p5','-rc','vbr','-cq','24','-b:v','850k'] if a.encoder=='h264_nvenc' else ['-c:v','libx264','-preset','fast','-crf','23','-threads','4']
 args=[shutil.which('ffmpeg'),'-v','error','-y','-i',str(source),'-vf','scale=720:1280:flags=lanczos,setsar=1','-an']+enc+['-maxrate','1100k','-bufsize','2200k','-pix_fmt','yuv420p','-movflags','+faststart',str(tmp)]
 subprocess.run(args,check=True,timeout=900)
 if tmp.stat().st_size>=100*1024*1024:raise SystemExit('Phone copy exceeds Drive connector limit; existing output preserved')
 subprocess.run([shutil.which('ffmpeg'),'-v','error','-i',str(tmp),'-f','null','-'],check=True,timeout=300)
 tmp.replace(out)
 data={'source':str(source),'sourceSHA256':manifest['outputSHA256'],'output':str(out),'outputSHA256':hashlib.sha256(out.read_bytes()).hexdigest(),'bytes':out.stat().st_size,'size':[720,1280],'speed':1,'captionRevision':2}
 (R/'work/tutorial-video/phone-manifest.json').write_text(json.dumps(data,indent=2));print(out)
if __name__=='__main__':main()
