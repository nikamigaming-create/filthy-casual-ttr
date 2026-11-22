. "$PSScriptRoot\runtime-helpers.ps1"
function Grab-PhysicalPart([uint32]$PartId,[int]$MoveMs=180,[switch]$NativeReload){
 $recoverBefore=(Get-Addon).physicalMags.recovered
 Send-XR @('grip l 0');Start-Sleep -Milliseconds 100
 for($attempt=0;$attempt -lt 7;$attempt++){
  $a=Get-Addon
  if($a.nativeGrabRef -eq $PartId -or ($NativeReload -and $a.physicalMags.recovered -gt $recoverBefore)){return}
  $p=@($a.physicalParts|Where-Object ref -eq $PartId)[0]
  if(!$p){throw 'Physical part is not loaded'}
  if($p.nativeMeshNear){
   Send-XR @('grip l 1');Start-Sleep -Milliseconds 180
   $check=Get-Addon;if($check.nativeGrabRef -eq $PartId -or ($NativeReload -and $check.physicalMags.recovered -gt $recoverBefore)){return}
   Send-XR @('grip l 0');Start-Sleep -Milliseconds 100
  }
  # Correct from the native contact point, never the animated palm or stale
  # eye origin. Stay above the surface instead of punching through its center.
  $a=Get-Addon;$p=@($a.physicalParts|Where-Object ref -eq $PartId)[0]
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $offset=.025
  $px=$s.handL.pos[0]+$p.tracking[0]-$a.nativeGripTracking[0][0]
  $py=$s.handL.pos[1]+$p.tracking[1]-$a.nativeGripTracking[0][1]+$offset
  $pz=$s.handL.pos[2]+$p.tracking[2]-$a.nativeGripTracking[0][2]
  Send-XR @('grip l 0');Start-Sleep -Milliseconds 100
  Send-XR @(("hand l grip pose {0:F5} {1:F5} {2:F5} -4 55 -6" -f $px,$py,$pz),'grip l 1')
  Start-Sleep -Milliseconds ($MoveMs+80)
 }
 $a=Get-Addon;if($a.nativeGrabRef -eq $PartId -or ($NativeReload -and $a.physicalMags.recovered -gt $recoverBefore)){return};$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-contact-failure.json"
 throw "Native grip did not acquire part $PartId"
}