# Actual native free-hand gesture, measured bone direction, slow curved approach.
function Point-TutorialPart([string]$Node){
 Send-XR @('trigger l 0','touch l index off','thumbrest l on','grip l 1')
 Start-Sleep -Milliseconds 650
 for($stage=0;$stage -lt 3;$stage++){
  $a=Get-Addon
  if($a.nativeGrabRef -ne 0){throw 'Pointing hand unexpectedly holds an object'}
  $p=(& python.exe "$PSScriptRoot\tutorial-point-pose.py" $Node)|ConvertFrom-Json
  if(!$p){throw 'No measured pointing pose'}
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $pos=if($stage -eq 0){$s.handL.pos}else{$p.position}
  Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 1500 -0.01 0.025 0" -f $pos[0],$pos[1],$pos[2],$p.angles[0],$p.angles[1],$p.angles[2]))
  Start-Sleep -Milliseconds 1650
 }
 Start-Sleep -Seconds 2
}
