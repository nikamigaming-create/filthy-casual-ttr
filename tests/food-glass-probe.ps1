. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','profile touch','head pose 0 1.1 0 0 -25 0','btn a press 180')
Send-Game @('load FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('tgm','tai','player.SetWeaponOut 0','set GameHour to 12','player.SetPos X -54170','player.SetPos Y -78170','player.SetPos Z 5400')|Out-Null
Start-Sleep -Seconds 3
foreach($food in '001613C5','0001519B','00015163','00032C75'){
 Park-Hands
 Write-Output "Item $food old food flag = $(Get-Native ('GetIngestibleFlag '+$food+' 2'))"
 $initial=Get-Native "player.GetItemCount $food"
 Send-Game @("player.AddItem $food 2",'sc_aid')|Out-Null;Start-Sleep -Milliseconds 350
 Send-Game @("sc_pick $food")|Out-Null;Start-Sleep -Milliseconds 1300
 $a=Get-Addon
 Assert-Equal $a.heldFood $true "$food eligible as native ingestible"
 Assert-Equal $a.nativeGrabHand 1 "$food uses right hand"
 Send-XR @('hand r to grip .45 .65 -.65 0 0 0 700');Start-Sleep -Milliseconds 950
 Save-Eyes "food-glass-$food-before"
 for($i=0;$i -lt 5;$i++){
  $a=Get-Addon;if(!$a.heldFood){break}
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $p=@();for($j=0;$j -lt 3;$j++){$p+=($s.handR.pos[$j]+$a.mouthTracking[$j]-$a.heldTracking[$j])}
  Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 0 0 0 600" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1000
 }
 $a=Get-Addon
 Assert-Equal $a.heldFood $false "$food consumed through mouth gesture"
 Assert-Equal (Get-Native "player.GetItemCount $food") ($initial+1) "$food consumed exactly once"
 Save-Eyes "food-glass-$food-after"
 # Return any real empty bottle, keeping the next case independent.
 if($a.nativeGrabRef){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
}
Park-Hands
$fn='Call (CompileScript "FCBottleImpact.txt") '
foreach($spot in @(@('reported-desert',-54170,-78170,5400),@('original-floor',-72500,2000,6000))){
 Send-Game @("player.SetPos X $($spot[1])","player.SetPos Y $($spot[2])","player.SetPos Z $($spot[3])")|Out-Null;Start-Sleep -Seconds 4
 Send-XR @('head pose 0 1.1 0 0 -65 0')
 $before=(Get-Addon).bottleBreaks
 $id=('{0:X8}' -f [int32](Get-Native ($fn+'"" 0 220 0 0 80')))
 Send-Game @("sc_track_bottle $id")|Out-Null
 Start-Sleep -Milliseconds 1800
 Send-Game @('sc_glass_state')|Out-Null
 $g=@(Get-Content "$script:Session\glass-state.json" -Raw|ConvertFrom-Json)[0]
 $dx=$g.tracking[0];$dy=$g.tracking[1]-1.1;$dz=$g.tracking[2]
 $yaw=[Math]::Atan2(-$dx,-$dz)*180/[Math]::PI
 $pitch=[Math]::Atan2($dy,[Math]::Sqrt($dx*$dx+$dz*$dz))*180/[Math]::PI
 Send-XR @(("head pose 0 1.1 0 {0:F3} {1:F3} 0" -f $yaw,$pitch));Start-Sleep -Milliseconds 300
 Save-Eyes "glass-$($spot[0])-2s"
 Assert-Equal (Get-Addon).bottleBreaks ($before+1) "$($spot[0]) detected native impact"
 Start-Sleep -Seconds 2;Save-Eyes "glass-$($spot[0])-4s"
 Start-Sleep -Seconds 8;Save-Eyes "glass-$($spot[0])-12s"
}
