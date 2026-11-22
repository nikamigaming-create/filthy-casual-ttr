. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -10 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('player.SetWeaponOut 0','set GameHour to 12','player.SetPos X -72500','player.SetPos Y 2000','player.SetAngle Z 0')|Out-Null;Start-Sleep -Seconds 2
$car=Get-Native 'player.PlaceAtMe (GetFormFromMod "UmDrivingVIIIb.esp" "00E6BC") 1 300 0'
Assert-Equal ($car -ne 0) $true 'Native roadster spawned from original car mod'
$id=('{0:X8}' -f $car)
$fn='Call (CompileScript "FCVehicleTest.txt") "'+$id+'" '
Get-Native ($fn+'4')|Write-Output
Get-Native ($fn+'5')|Write-Output
Get-Native ($fn+'3')|Write-Output
Start-Sleep -Seconds 2
$a=Get-Addon
$rx=$a.nativeGripPoints[1][0]-$a.nativeGripPoints[0][0];$ry=$a.nativeGripPoints[1][1]-$a.nativeGripPoints[0][1]
$heading=[Math]::Atan2(-$ry,$rx)*180/[Math]::PI
Get-Native ('Call (CompileScript "FCVehicleAlign.txt") "'+$id+'" '+$heading.ToString('F3',[Globalization.CultureInfo]::InvariantCulture))|Write-Output
Start-Sleep -Milliseconds 500
Get-Native ($fn+'1')|Write-Output;Start-Sleep -Seconds 3
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 3 'Native vehicle driving state entered'
# Combat unlock may restore the fixture's previously equipped weapon. Holster
# it explicitly before testing two empty hands on the rim.
Send-Game @('player.SetWeaponOut 0')|Out-Null;Start-Sleep -Milliseconds 600
Park-Hands

if(Get-Command Mode-Check -ErrorAction SilentlyContinue){Mode-Check 'vehicles.driving' 'robco-driving-active'}
function Wheel-State { Get-Content "$script:Session\vehicle-state.json" -Raw|ConvertFrom-Json }
$w=Wheel-State
$yaw=[Math]::Atan2(-$w.center[0],-$w.center[2])*180/[Math]::PI
Send-XR @("head to 0 1.1 0 $yaw -25 0 700","hand l grip rot $yaw 10 0","hand r grip rot $yaw 10 0",'touch l index on','touch r index on','thumbrest l on','thumbrest r on')
Start-Sleep -Milliseconds 800
function Move-Rim([double]$Degrees,[int]$Duration=600){
 $w=Wheel-State;$theta=$Degrees*[Math]::PI/180
 for($hand=0;$hand -lt 2;$hand++){
  $sign=if($hand -eq 0){-1}else{1};$h=if($hand -eq 0){'l'}else{'r'}
  $t=@();for($j=0;$j -lt 3;$j++){$t+=($w.center[$j]+$sign*([Math]::Cos($theta)*($w.right[$j]-$w.center[$j])+[Math]::Sin($theta)*($w.up[$j]-$w.center[$j])))}
  $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $pose=if($hand -eq 0){$state.handL.pos}else{$state.handR.pos}
  $p=@();for($j=0;$j -lt 3;$j++){$p+=($pose[$j]+$t[$j]-$w.palms[$hand][$j])}
  Send-XR @(("hand {0} to grip {1:F5} {2:F5} {3:F5} {4:F3} 10 0 {5}" -f $h,$p[0],$p[1],$p[2],$yaw,$Duration))
 }
 Start-Sleep -Milliseconds ($Duration+250)
}
for($i=0;$i -lt 3 -and (Get-Native 'MenuMode');$i++){Send-XR @('btn a press 150');Start-Sleep -Milliseconds 500}
Assert-Equal (Get-Native 'MenuMode') 0 'Driving is live before physical rim contact'
# Approach from above the dashboard, following a reachable physical hand path.
Send-XR @('hand l to grip -.2 1.15 -.15 0 10 0 800','hand r to grip .2 1.15 -.15 0 10 0 800');Start-Sleep -Milliseconds 1000
Send-XR @('hand l to grip -.2 1.15 -.42 0 10 0 800','hand r to grip .2 1.15 -.42 0 10 0 800');Start-Sleep -Milliseconds 1000
Move-Rim 0
Move-Rim 0 200
Send-XR @('grip l .9','grip r .9');Start-Sleep -Milliseconds 350
# Attachment is now locked; do not chase the rendered hand with controller input.
Start-Sleep -Milliseconds 400
$w=Wheel-State;$w|ConvertTo-Json -Depth 6|Write-Output
Assert-Equal ($w.grabbed[0] -eq 1 -and $w.grabbed[1] -eq 1) $true 'Both tracked hands grip the steering rim'
Assert-Equal (Get-Native 'MenuMode') 0 'Reaching and gripping does not open Pause'
Send-XR @('stick r .6 0');Start-Sleep -Milliseconds 300
Assert-Equal ((Wheel-State).steer -gt .55) $true 'Thumbstick overrides physical wheel grip'
Send-XR @('stick r 0 0','stick l 0 0')
. "$PSScriptRoot\casual-guide-helpers.ps1"
$script:CaptureAddonPath='D:\TTWVR\MO2\mods\TTWVR - RobCo Vehicle Test\NVSE\Plugins\SeatedControls.dll'
Start-Take 'vehicle-contact-motion' 105
Mark 'Both hands gripping'
Save-Eyes 'vehicle-wheel-gripped'

Move-Rim 40 1000
$turn=Wheel-State
Assert-Equal ([Math]::Abs($turn.steer) -gt .25) $true 'Turning the physical wheel produces native steering'
Save-Eyes 'vehicle-wheel-turned'
$start=(Wheel-State).position
Send-XR @('stick l 0 .65');Start-Sleep -Seconds 2
$drive=Wheel-State
Assert-Equal ($drive.gas -gt .4) $true 'Right stick forward reaches native throttle'
Assert-Equal ([Math]::Abs($drive.position[0]-$start[0])+[Math]::Abs($drive.position[1]-$start[1]) -gt 10) $true 'Native car physics moves the vehicle'
Mark 'Brake, release right hand, keep left on rim'
Send-XR @('stick r 0 0','stick l 0 0');Start-Sleep -Seconds 2
Send-XR @('grip r 0','hand r to grip .4 .75 -.15 0 30 0 700');Start-Sleep -Milliseconds 900
$w=Wheel-State
Assert-Equal $w.grabbed[0] 1 'Left rim attachment survives right-hand release'
Assert-Equal $w.grabbed[1] 0 'Right hand is actually free'
Mark 'Head turn with one hand holding'
Send-XR @('head to 0 1.1 0 -20 -25 0 1200');Start-Sleep -Milliseconds 1400
Send-XR @('head to 0 1.1 0 20 -25 0 1600');Start-Sleep -Milliseconds 1800
Send-XR @('head to 0 1.1 0 0 -25 0 1000');Start-Sleep -Milliseconds 1200
# A normal selector draw, then real trigger queries; no ammo writes.
Mark 'Free right hand draws and fires a personal weapon'
Send-Game @('player.AddItem 0000434F 1','player.AddItem 00004241 40','sc_weapons')|Out-Null;Start-Sleep -Milliseconds 300
Send-Game @('sc_pick 0000434F')|Out-Null;Start-Sleep -Seconds 2
Send-XR @('hand r to grip .22 .93 -.44 0 55 0 800');Start-Sleep -Milliseconds 1000
$a=Get-Addon;$a|ConvertTo-Json -Depth 18|Set-Content "$script:Repo\work\vehicle-controls\gun-before.json"
Assert-Equal $a.weaponHand 1 'Selector weapon retained in the right hand'
$ammo0=Get-Native 'player.GetCurrentAmmoRounds'
$bag0=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
Write-Output "Before firing: clip=$ammo0 bag=$bag0 drawn=$(Get-Native 'player.IsWeaponOut') combatDisabled=$(Get-Native 'GetPlayerControlsDisabled 0 1 0 0 0 0 0')"
$start=(Wheel-State).position
Send-XR @('stick l 0 .5');Start-Sleep -Milliseconds 600
Send-XR @('trigger r 1');Start-Sleep -Milliseconds 400;Send-Game @('sc_controls_state')|Out-Null;Copy-Item "$script:Session\controls-state.json" "$script:Repo\work\vehicle-controls\shoot-down-input.json" -Force;Send-XR @('trigger r 0');Start-Sleep -Milliseconds 450
$ammo1=Get-Native 'player.GetCurrentAmmoRounds'
$bag1=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
$a=Get-Addon;$a|ConvertTo-Json -Depth 18|Set-Content "$script:Repo\work\vehicle-controls\gun-after.json"
Send-Game @('sc_controls_state')|Write-Output
Write-Output "After firing: clip=$ammo1 bag=$bag1 drawn=$(Get-Native 'player.IsWeaponOut')"
Save-Eyes 'vehicle-free-hand-weapon' 
Assert-Equal ($ammo1 -lt $ammo0 -or $bag1 -lt $bag0) $true 'Free hand fires native gun while driving'
Assert-Equal ([Math]::Abs((Wheel-State).gas-(.5-.15)/.85) -lt .01) $true 'Gun trigger leaves stick-controlled throttle unchanged'
Assert-Equal ([Math]::Abs((Wheel-State).position[0]-$start[0])+[Math]::Abs((Wheel-State).position[1]-$start[1]) -gt 10) $true 'Car keeps moving while the native weapon fires'
Save-Eyes 'vehicle-free-hand-weapon'
Send-Game @('player.SetWeaponOut 0')|Out-Null
Send-XR @('grip l 0','grip r 0','stick r .5 0','stick l 0 .5');Start-Sleep -Milliseconds 700
Save-Eyes 'vehicle-knees-driving'
Assert-Equal ((Wheel-State).knees -gt .95) $true 'Moving with both hands free raises the knees'
Assert-Equal ((Wheel-State).steer -gt .45) $true 'Same right stick steers with both hands free'
Send-XR @('stick l 0 -1');Start-Sleep -Milliseconds 250
$deadline=[DateTime]::UtcNow.AddSeconds(7)
do{Start-Sleep -Milliseconds 200;$gear=Get-Native ('Call (CompileScript "FCVehicleReverse.txt") (StringToRef "'+$id+'")')}while($gear -ne 1 -and [DateTime]::UtcNow -lt $deadline)
Assert-Equal $gear 1 'Down stick brakes then selects reverse'
Assert-Equal ((Wheel-State).gas -gt .9) $true 'Down stick drives backward after stopping'
Send-XR @('stick r 0 0','stick l 0 0');Start-Sleep -Seconds 2
Finish-Take
Stop-Take

Send-XR @('grip l 0','grip r 0','trigger l 0','btn b press 200');Start-Sleep -Seconds 2
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 0 'Quest B exits the native vehicle'
Assert-Equal (Get-Native 'player.GetDead') 0 'Driver survives the drive'
