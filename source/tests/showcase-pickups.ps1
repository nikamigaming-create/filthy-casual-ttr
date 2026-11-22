. "$PSScriptRoot\casual-guide-helpers.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\human-showcase'
New-Item -ItemType Directory -Force $script:Guide|Out-Null
function Relax {
 Send-XR @('hand l curve grip -0.23 0.78 -0.20 -8 55 -8 1400 -0.02 0.04 0','hand r curve grip 0.22 0.80 -0.23 8 55 8 1500 0.02 0.04 0')
 Start-Sleep -Milliseconds 1700
}
function Wheel([string]$h){
 Relax
 $other=if($h -eq 'l'){'r'}else{'l'}
 Send-XR @('input clear',"thumbrest $other on");Start-Sleep -Milliseconds 200
 Send-XR @("thumbrest $h on");Start-Sleep -Milliseconds 90
 Send-XR @("thumbrest $h off");Start-Sleep -Milliseconds 110
 Send-XR @("thumbrest $h on");Start-Sleep -Milliseconds 180
 Assert-Equal (Get-Addon).openHand $(if($h -eq 'l'){0}else{1}) 'Showcase wheel opens'
 Send-XR @('input clear')
 Start-Sleep -Seconds 3
}
function Reach([int]$slot){
 $a=Get-Addon
 $target=@($a.previews|Where-Object slot -eq $slot)[0].center
 $dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1]
 $len=[Math]::Sqrt($dx*$dx+$dy*$dy);if($len -lt .01){throw 'Invalid preview basis'}
 $rx=$dx/$len;$ry=$dy/$len
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $ex=$target[0]-$a.palms[1][0];$ey=$target[1]-$a.palms[1][1];$ez=$target[2]-$a.palms[1][2]
 $px=$s.handR.pos[0]+($rx*$ex+$ry*$ey)/70
 $py=$s.handR.pos[1]+$ez/70
 $pz=$s.handR.pos[2]-(-$ry*$ex+$rx*$ey)/70
 Send-XR @('touch r index off','thumbrest r on',("hand r curve grip {0:F5} {1:F5} {2:F5} 5 55 4 2100 0.025 0.075 0.025" -f $px,$py,$pz))
 Start-Sleep -Milliseconds 2400
 if((Get-Addon).openHand -ge 0){Touch-Slot $slot 1 @($rx,$ry)|Out-Null}
 Assert-Equal (Get-Addon).openHand -1 'Touch selects without a grab'
}
function Present {
 Send-XR @('hand r curve grip 0.17 0.94 -0.37 4 55 8 1600 0.015 0.04 0','head to 0 1.1 0 3 -18 0 1200')
 Start-Sleep -Milliseconds 1900
}

Load-Guide
Assert-Equal (Get-Addon).heightOffset 0.55 'Saved seated height restored after process restart and load'
Send-Game @('player.SetWeaponOut 0')|Out-Null
Send-XR @('input clear','head to 0 1.1 0 0 -32 0 1200');Relax
Start-Take '04-height-return' 15
try {
 Mark 'Saved height restored';Start-Sleep -Seconds 2
 Send-XR @('head to 0 1.1 0 0 -12 0 1800');Start-Sleep -Seconds 3
 Finish-Take
}finally{Stop-Take}
$script:events.Clear()
Wheel 'r'
$p=@((Get-Addon).previews|Where-Object form -eq 17231)
if(!$p.Count){throw 'Pistol absent from fixture'}
Reach $p[0].slot;Present
Assert-Equal (Get-Addon).weaponHand 1 'Weapon begins in convenience hold'
Start-Take '05-native-hold' 18
try {
 Mark 'Convenience hold';Start-Sleep -Seconds 2
 Mark 'Squeeze for native';Send-XR @('grip r 1');Start-Sleep -Milliseconds 700
 Assert-Equal (Get-Addon).weaponHand -1 'Deliberate squeeze returns weapon ownership to native TTR'
 Send-XR @('grip r 0','hand r curve grip 0.23 0.80 -0.24 8 55 8 1800 0.02 0.03 0');Start-Sleep -Seconds 3
 Assert-Equal (Get-Addon).weaponHand -1 'Addon does not reclaim native weapon holding'
 Finish-Take
}finally{Stop-Take}
$script:events.Clear()
Send-Game @('player.SetWeaponOut 0','player.AddItem 0001519B 3','player.damageav health 40')|Out-Null
$foodCount=Get-Native 'player.GetItemCount 0001519B'
Wheel 'l';$food=$null
for($page=0;$page -lt 5;$page++){
 $a=Get-Addon;$food=@($a.previews|Where-Object form -eq 86427)
 if($food.Count){break}
 Send-XR @('btn a press 180');Start-Sleep -Milliseconds 450
}
if(!$food.Count){throw 'Iguana food absent from fixture'}
$dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1]
$len=[Math]::Sqrt($dx*$dx+$dy*$dy);$rx=$dx/$len;$ry=$dy/$len
Reach $food[0].slot;Present
Assert-Equal (Get-Addon).heldFood $true 'Selected item is native food'
Assert-Equal (Get-Addon).aidGripLatch $true 'Mouth gesture begins only in convenience hold'
Start-Take '06-food-mouth' 20
try {
 Mark 'Held food';Start-Sleep -Seconds 2
 Mark 'Bring food to mouth'
 for($i=0;$i -lt 3;$i++){
  $a=Get-Addon;if(!$a.nativeGrabRef){break}
  $xr=Get-Content "$script:Session/state.json" -Raw|ConvertFrom-Json
  $ex=$a.mouthPoint[0]-$a.heldCenter[0];$ey=$a.mouthPoint[1]-$a.heldCenter[1];$ez=$a.mouthPoint[2]-$a.heldCenter[2]
  $px=$xr.handR.pos[0]+($rx*$ex+$ry*$ey)/70
  $py=$xr.handR.pos[1]+$ez/70
  $pz=$xr.handR.pos[2]-(-$ry*$ex+$rx*$ey)/70
  Send-XR @(("hand r curve grip {0:F5} {1:F5} {2:F5} 4 55 8 1600 0.015 0.03 0" -f $px,$py,$pz))
  Start-Sleep -Milliseconds 2200
 }
 Assert-Equal (Get-Addon).nativeGrabRef 0 'Food consumed by the mouth gesture'
 Assert-Equal (Get-Native 'player.GetItemCount 0001519B') ($foodCount-1) 'Exactly one actual food item consumed'
 Mark 'Native food consumed';Relax;Start-Sleep -Seconds 1
 Finish-Take
}finally{Stop-Take}
