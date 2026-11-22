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
Send-Game @('player.SetWeaponOut 0')|Out-Null
Send-XR @('input clear','head to 0 1.1 0 0 -12 0 800');Relax
Assert-Equal (Get-Addon).heightOffset 0 'Seated showcase starts before calibration'
Start-Take '01-height-weapons' 115
try{
 Mark 'Seated low';Start-Sleep -Seconds 3
 Send-XR @('head to 0 1.1 0 -3 -48 -1 1800');Start-Sleep -Seconds 4
 Mark 'Open height';Send-XR @('btn menu down');Start-Sleep -Milliseconds 1250;Send-XR @('btn menu up');Start-Sleep -Seconds 2
 Assert-Equal (Get-Addon).heightMenu $true 'Menu hold opens seated settings'
 Mark 'Calibrate';Send-XR @('btn a press 180');Start-Sleep -Seconds 4
 Assert-Equal (Get-Addon).heightOffset 0.55 'Recorded calibration is real'
 Send-XR @('btn b press 180');Start-Sleep -Seconds 3
 Send-XR @('head to 0 1.1 0 0 -12 0 1700');Start-Sleep -Seconds 2
 Mark 'Weapons wheel';Wheel 'r'
 $p=@((Get-Addon).previews|Where-Object form -eq 17231)
 if(!$p.Count){throw 'Pistol absent from fixture'}
 Mark 'Touch weapon';Reach $p[0].slot;Present;Start-Sleep -Seconds 4
 Assert-Equal (Get-Native 'player.GetEquippedObject 5') 17231 'Native pistol selected'
 # Load a nearly empty magazine in the isolated fixture before the shot segment.
 Send-Game @('player.SetCurrentAmmoRounds 1')|Out-Null
 Mark 'Automatic reload';Send-XR @('head to 0 1.1 0 0 -8 0 900','hand r curve grip 0.13 1.00 -0.48 0 55 3 1200 0.01 0.03 0','touch r index on');Start-Sleep -Seconds 2
 Send-XR @('trigger r pull 180');Start-Sleep -Seconds 4
 Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'Native automatic reload occurred'
 Mark 'Manual mode';Send-XR @('btn menu press 180');Start-Sleep -Seconds 3
 Assert-Equal (Get-Addon).manualReload $true 'Menu tap switches to manual'
 Send-XR @('head to 0 1.1 0 -2 -50 0 1500','hand l curve grip -0.35 0.66 -0.12 -10 55 -8 1300 -0.02 0.02 0','hand r curve grip 0.32 0.80 -0.28 8 55 8 1400 0.02 0.02 0');Start-Sleep -Seconds 4
 Mark 'Back to auto';Send-XR @('btn menu press 180');Start-Sleep -Seconds 2
 Assert-Equal (Get-Addon).manualReload $false 'Menu tap returns to automatic'
 Send-XR @('head to 0 1.1 0 0 -12 0 1300');Start-Sleep -Milliseconds 1400
 Mark 'Clear weapon';Wheel 'r';Reach 6;Relax;Start-Sleep -Seconds 3
 Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Center cap clears weapon'
 Finish-Take
}finally{Stop-Take}
$script:events.Clear()
Send-XR @('input clear','head to 0 1.1 0 0 -12 0 800');Relax
Start-Take '02-aid-gestures' 115
try{
 Mark 'Consumables wheel';Wheel 'l'
 $a=Get-Addon;$p=@($a.previews|Where-Object {$_.slot -ne 6 -and $_.form -ne 86377})[0]
 if(!$p){throw 'Ordinary aid absent'}
 Mark 'Touch consumable';Reach $p.slot;Present;Start-Sleep -Seconds 5
 Assert-Equal (Get-Addon).nativeGrabHand 1 'Aid held in native right hand'
 Mark 'Clear consumable';Wheel 'l';Reach 6;Relax;Start-Sleep -Seconds 3
 Assert-Equal (Get-Addon).nativeGrabRef 0 'Center cap stows aid'
 Mark 'Hands alive';Send-XR @('input clear','head to 0 1.1 0 0 -34 0 1400','hand l curve grip -0.20 0.94 -0.31 -6 55 -8 1800 -0.025 0.07 0','hand r curve grip 0.20 0.95 -0.32 6 55 8 1900 0.025 0.07 0');Start-Sleep -Seconds 3
 Mark 'Gentle finger curl'
 Send-XR @('thumbrest l on','thumbrest r on','touch l index on','touch r index on')
 foreach($g in .15,.35,.60,.85,1,.85,.55,.25,0){Send-XR @("grip l $g","grip r $g");Start-Sleep -Milliseconds 130}
 Start-Sleep -Milliseconds 500
 Mark 'Point';Send-XR @('grip r 1','touch r index off','thumbrest r on','hand r curve grip 0.16 0.99 -0.40 0 55 -5 1400 0.01 0.03 0');Start-Sleep -Seconds 3
 Send-XR @('hand r curve grip 0.22 0.99 -0.39 8 55 5 1700 0.0 0.02 0');Start-Sleep -Seconds 2
 Mark 'Thumbs up';Send-XR @('touch r index on','thumbrest r off','touch r stick off','touch r primary off','touch r secondary off','hand r curve grip 0.19 0.99 -0.32 -4 55 -16 1500 0.015 0.04 0');Start-Sleep -Seconds 4
 Assert-Equal ((Get-Addon).fingerGestures[1].lift -gt .98) $true 'Thumbs up reached'
 Mark 'Relaxed wave';Send-XR @('grip r 0','touch r index off','hand r curve grip 0.25 1.00 -0.32 8 55 -50 1300 0.02 0.03 0');Start-Sleep -Milliseconds 1500
 Send-XR @('hand r curve grip 0.17 1.00 -0.33 -8 55 -35 1200 -0.02 0.02 0');Start-Sleep -Milliseconds 1400
 Relax;Start-Sleep -Seconds 3
 Finish-Take
}finally{Stop-Take}
