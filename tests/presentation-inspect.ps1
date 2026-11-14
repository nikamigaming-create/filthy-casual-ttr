. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 6
for($notice=0;$notice -lt 3 -and (Get-Addon).nativeTopMenu -eq 1001;$notice++){
 Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600
}
Park-Hands
Send-Game @('player.MoveTo 00104C79 110 0 0','player.SetAngle z 270')|Out-Null
Start-Sleep -Seconds 2
Save-Eyes 'presentation-dialogue-room'
Send-Game @('expr Call ({ref actor} => actor.Activate player) (StringToRef "00104C79")')|Out-Null
Start-Sleep -Seconds 4
$a=Get-Addon
Write-Output ($a | Select-Object menu,nativeTopMenu,dialoguePlacements,dialogueSideDegrees | ConvertTo-Json -Compress)
$dx=$a.speakerTracking[0]-$a.nativeHeadTracking[0]
$dz=$a.speakerTracking[2]-$a.nativeHeadTracking[2]
$faceYaw=[Math]::Atan2(-$dx,-$dz)*180/[Math]::PI
Send-XR @("head pose 0 1.1 0 $faceYaw 0 0")
Start-Sleep -Milliseconds 500
Save-Eyes 'presentation-dialogue-near'
$turnYaw=$faceYaw-$a.dialogueSideDegrees*.5
Send-XR @("head to 0.05 1.1 0 $turnYaw 0 0 700")
Start-Sleep -Seconds 1
Save-Eyes 'presentation-dialogue-turn'
Send-Game @('CloseAllMenus')|Out-Null
Send-XR @('head pose 0 1.1 0 0 0 0')
Send-Game @('load SC_QA_scope_autosave')|Out-Null
Start-Sleep -Seconds 6
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
foreach($kind in 28,42,43){Write-Output "Nearby native type $kind : $(Get-Native "GetFirstRef $kind 1")"}
foreach($weapon in @('00106FEB')) {
 Park-Hands
 Send-Game @("player.AddItem $weapon 1",'sc_weapons')|Out-Null
 Start-Sleep -Milliseconds 350
 Send-Game @("sc_pick $weapon")|Out-Null
 Start-Sleep -Milliseconds 800
 $a=Get-Addon
 Write-Output ($a | Select-Object nativeScopeActive,nativeScopeFrames,lensSamples,lensTracking | ConvertTo-Json -Compress)
 # Align the actual native lens to the right eye; do not merely assume the
 # grip origin is the scope eye point. Height correction is included in stage.
 $hx=.04;$hy=.93;$hz=-.35
 foreach($pitch in 35,45,55,65,75){
  Send-XR @("hand r grip pose $hx $hy $hz 0 $pitch 0")
  Start-Sleep -Milliseconds 500
  $a=Get-Addon
  if($a.lensSamples){
   $hx+=.0315-$a.lensTracking[0]
   $hy+=(1.1+$a.heightOffset)-$a.lensTracking[1]
   $hz+=-.35-$a.lensTracking[2]
   Send-XR @("hand r grip pose $hx $hy $hz 0 $pitch 0")
   Start-Sleep -Milliseconds 900
   $a=Get-Addon
  }
  Write-Output ("Scope pitch=$pitch pose=$hx,$hy,$hz "+($a | Select-Object nativeScopeActive,nativeScopeFrames,lensSamples,lensTracking | ConvertTo-Json -Compress))
  if($a.nativeScopeActive){Save-Eyes 'presentation-scope-active';break}
 }
 foreach($schedule in 'native','guard'){
  Send-Game @("sc_scope_$schedule")|Out-Null
  Send-XR @("hand r grip pose $hx $hy $hz 0 $pitch 0")
  Start-Sleep -Seconds 2
  Write-Output ("Surface $schedule "+((Get-Addon)|Select-Object nativeScopeActive,nativeScopeFrames,scopeSurfaceAttempts,scopeSurfaceRestored,scopeSurfaceFailures,scopeSurfaceHRESULT,scopeSurfaceSize,scopeSurfaceSamples |ConvertTo-Json -Compress))
  Save-Eyes "presentation-scope-$schedule"
 }
 Send-Game @('sc_scope_inline')|Out-Null
 Send-XR @("hand r grip pose $hx $hy $hz 0 $pitch 0",'hand l grip pose -0.02 0.93 -0.58 0 55 0','grip l 1','video on')
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $record=Start-Process -FilePath 'C:\Users\nbrys\AppData\Local\Programs\Python\Python311\python.exe' -ArgumentList @("`"$PSScriptRoot\record-eyes.py`"",'--pid',$s.pid,'--out',"`"$script:Repo\evidence\presentation-scope-$weapon.mp4`"",'--seconds',14,'--fps',30) -WindowStyle Hidden -PassThru
 try {
  $i=0
  foreach($pose in @("$hx $hy $hz 0 $pitch 0","$hx $hy $($hz-.15) -10 $pitch -15","$hx $hy $($hz-.15) 10 $pitch 15","$hx $hy $($hz-.35) -10 $pitch -15","$hx $hy $($hz-.35) 10 $pitch 15")) {
   Send-XR @("hand r to grip $pose 1300")
   Start-Sleep -Milliseconds 1700
   $i++
  }
  if(!$record.WaitForExit(7000)){throw 'Scope capture exceeded duration'}
 } finally {Send-XR @('video off','grip l 0');if(!$record.HasExited){Stop-Process -Id $record.Id -Force}}
}
Park-Hands
$target=Get-Native 'GetFirstRef 43 1'
if($target){
 $ref='{0:X8}' -f [int]$target
 # Disposable unsaved fixture only: exercise an actual body/loot transition.
 Send-Game @(('expr Call ({ref actor} => actor.Kill player) (StringToRef "'+$ref+'")'),"player.MoveTo $ref 0 100 0",'player.SetAngle z 180')|Out-Null
 foreach($item in @('00015169','00015168','00015167','00015166','00015165','00015163','00015162','00015161','00015160','0001515F','0001515E','0001515D','0001515C','0000000F')){
  Send-Game @(('expr Call ({ref actor} => actor.AddItem (StringToRef "'+$item+'") 3) (StringToRef "'+$ref+'")'))|Out-Null
 }
 Start-Sleep -Seconds 2
 Send-Game @('player.UnequipItem 00106FEB',"sc_target $ref")|Out-Null
 $probe=Get-Content "$script:Session\target-probe.json" -Raw|ConvertFrom-Json
 if(!$probe.ok){throw 'Creature target bounds unavailable'}
 $a=Get-Addon;$p=$probe.tracking
 Write-Output ("Creature target "+($probe|ConvertTo-Json -Compress))
 $dx=$p[0]-.2;$dy=$p[1]-(.8+$a.heightOffset);$dz=$p[2]+.2
 $yaw=[Math]::Atan2(-$dx,-$dz)*180/[Math]::PI
 $pitch=[Math]::Atan2($dy,[Math]::Sqrt($dx*$dx+$dz*$dz))*180/[Math]::PI
 Send-XR @('hand l valid off','hand r aimtrim 0 0',"head pose 0 1.1 0 $yaw -25 0","hand r grip pose .2 .8 -.2 $yaw $pitch 0")
 Start-Sleep -Seconds 2
 $a=Get-Addon
 Write-Output ("Creature $ref "+($a|Select-Object menu,nativeTopMenu,worldHud|ConvertTo-Json -Depth 6 -Compress))
 $hud=$a.worldHud.tracking
 $hyaw=[Math]::Atan2(-$hud[0],-$hud[2])*180/[Math]::PI
 $hpitch=[Math]::Atan2($hud[1]-1.1-$a.heightOffset,[Math]::Sqrt($hud[0]*$hud[0]+$hud[2]*$hud[2]))*180/[Math]::PI
 Send-XR @("head pose 0 1.1 0 $hyaw $hpitch 0")
 $anchor=$a.worldHud.paths[1].world -join ','
 $trackingBefore=$a.worldHud.tracking -join ','
 $samples=$a.worldHud.paths[1].samples
 Save-Eyes 'presentation-animal'
 foreach($offset in @(-2,2,-1,1,0)){
  Send-XR @("hand r grip pose .2 .8 -.2 $($yaw+$offset) $pitch 0")
  Start-Sleep -Milliseconds 300
  $a=Get-Addon
  Assert-Equal ($a.worldHud.paths[1].world -join ',') $anchor 'Native loot anchor remains fixed during controller motion'
  Assert-Equal ($a.worldHud.paths[1].samples -gt $samples) $true 'Moving controller continues native HUD publication'
 }
 Send-XR @('stick r 1 0');Start-Sleep -Milliseconds 220;Send-XR @('stick r center')
 Write-Output ("HUD page "+((Get-Addon).worldHud|ConvertTo-Json -Depth 5 -Compress))
 Assert-Equal ((Get-Addon).worldHud.pages -gt 0) $true 'Native loot horizontal paging'
 Assert-Equal ((Get-Addon).worldHud.tracking -join ',') $trackingBefore 'Loot paging does not snap-turn the world'
 Save-Eyes 'presentation-animal-page'
 Send-XR @('stick r -1 0');Start-Sleep -Milliseconds 220;Send-XR @('stick r center')
 Assert-Equal (Get-Addon).worldHud.selected 0 'Left pages back to the first native loot entry'
 Send-XR @('stick r 0 -1');Start-Sleep -Milliseconds 220;Send-XR @('stick r center')
 Assert-Equal (Get-Addon).worldHud.selected 1 'Down still moves one native loot row'
 Send-XR @("head to .08 1.13 0 $($hyaw+5) $hpitch 0 500")
 Start-Sleep -Milliseconds 650
 Assert-Equal ((Get-Addon).worldHud.paths[1].world -join ',') $anchor 'World HUD remains fixed through head translation and rotation'
 Save-Eyes 'presentation-animal-head'
}
