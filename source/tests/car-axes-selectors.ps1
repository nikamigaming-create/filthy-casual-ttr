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
function Wheel-State {Get-Content "$script:Session\vehicle-state.json" -Raw|ConvertFrom-Json}

Send-Game @('player.AddItem 0000434F 1','player.AddItem 00004241 60','player.AddItem 00032C75 2')|Out-Null
Send-XR @('input clear');Park-Hands;Start-Sleep -Milliseconds 400
Send-XR @('stick r 0 1');Start-Sleep -Milliseconds 600
Assert-Equal ((Wheel-State).gas -eq 0) $true 'Right stick Y does not drive the car'
Send-XR @('stick r .6 0');Start-Sleep -Milliseconds 400
Assert-Equal ((Wheel-State).steer -gt .55) $true 'Right stick X steers'
Send-XR @('stick r 0 0','stick l .8 0');Start-Sleep -Milliseconds 400
Assert-Equal ([Math]::Abs((Wheel-State).steer) -lt .01) $true 'Left stick X does not steer'
Send-XR @('stick l 0 .5');Start-Sleep -Milliseconds 600
Assert-Equal ((Wheel-State).gas -gt .4) $true 'Left stick Y drives forward'
function Open-MountedWheel([string]$h){
 Send-XR @('thumbrest l off','thumbrest r off');Start-Sleep -Milliseconds 120
 $other=if($h -eq 'l'){'r'}else{'l'}
 Send-XR @("thumbrest $other on");Start-Sleep -Milliseconds 180
 Send-XR @("thumbrest $h on");Start-Sleep -Milliseconds 90
 Send-XR @("thumbrest $h off");Start-Sleep -Milliseconds 110
 Send-XR @("thumbrest $h on");Start-Sleep -Milliseconds 180
 Send-XR @('thumbrest l off','thumbrest r off')
}
function Find-MountedItem([uint32]$form){
 for($page=0;$page -lt 8;$page++){
  $a=Get-Addon;$target=@($a.previews|Where-Object form -eq $form)|Select-Object -First 1
  if($target){return $target.slot}
  Send-XR @('btn a press 120');Start-Sleep -Milliseconds 350
 }
 throw 'Requested item missing from selector'
}
function Touch-MountedItem([int]$slot,[int]$hand=1){
 $h=if($hand){'r'}else{'l'}
 $side=if($hand){.45}else{-.45}
 Send-XR @("hand $h to grip $side .8 -.15 0 50 0 400");Start-Sleep -Milliseconds 550
 for($try=0;$try -lt 7;$try++){
  $a=Get-Addon;if($a.openHand -lt 0){break}
  $target=@($a.previews|Where-Object slot -eq $slot)[0].tracking
  $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  # Approach from the player's side of the chosen preview, then touch it.
  # A straight diagonal through the middle would correctly select the cap.
  if($try -lt 2){
   $toward=@(($state.head.pos[0]-$target[0]),($state.head.pos[1]+$a.heightOffset-$target[1]),($state.head.pos[2]-$target[2]))
   $length=[Math]::Sqrt($toward[0]*$toward[0]+$toward[1]*$toward[1]+$toward[2]*$toward[2])
   for($j=0;$j -lt 3;$j++){$target[$j]+=.22*$toward[$j]/$length}
  }
  $raw=if($hand){$state.handR.pos}else{$state.handL.pos}
  $pos=@();for($j=0;$j -lt 3;$j++){$pos+=($raw[$j]+$target[$j]-$a.selectorPalms[$hand][$j])}
  Send-XR @(("hand {3} to grip {0:F5} {1:F5} {2:F5} 0 50 0 350" -f $pos[0],$pos[1],$pos[2],$h));Start-Sleep -Milliseconds 500
 }
 Assert-Equal (Get-Addon).openHand -1 'Physical touch selects and closes mounted wheel'
 Start-Sleep -Milliseconds 1500
}
Open-MountedWheel r
Assert-Equal (Get-Addon).openHand 1 'Default thumb-rest combo opens weapons while driving'
$slot=Find-MountedItem 0x434F
$a=Get-Addon;$target=@($a.previews|Where-Object slot -eq $slot)[0].tracking;$start=(Wheel-State).position
Start-Sleep -Seconds 3
$a=Get-Addon
Assert-Equal $a.openHand 1 'Driving input and vehicle travel do not dismiss weapon wheel'
Assert-Equal $a.selectorMounted $true 'Weapon previews use vehicle anchor'
$now=@($a.previews|Where-Object slot -eq $slot)[0].tracking
$delta=0;for($j=0;$j -lt 3;$j++){$delta+=[Math]::Abs($now[$j]-$target[$j])}
Assert-Equal ($delta -lt .08) $true 'Weapon preview stays reachable while travelling'
Assert-Equal ([Math]::Abs((Wheel-State).position[0]-$start[0])+[Math]::Abs((Wheel-State).position[1]-$start[1]) -gt 10) $true 'Car actually travels with wheel open'
Save-Eyes 'car-moving-weapon-wheel'
Touch-MountedItem $slot
Assert-Equal (Get-Addon).weaponHand 1 'Touched weapon goes to right hand'
Assert-Equal (Get-Native 'player.GetEquippedObject 5') 0x434F 'Touched weapon is natively equipped'
Assert-Equal ((Wheel-State).gas -gt .4) $true 'Equipping does not cancel held driving input'
$before=Get-Native 'player.GetCurrentAmmoRounds'
Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 400
Assert-Equal ((Get-Native 'player.GetCurrentAmmoRounds') -lt $before) $true 'Selected weapon fires while driving'
Park-Hands
Open-MountedWheel l
Assert-Equal (Get-Addon).openHand 0 'Default thumb-rest combo opens consumables while driving'
$slot=Find-MountedItem 0x32C75
Start-Sleep -Seconds 2
Assert-Equal (Get-Addon).openHand 0 'Consumable wheel stays open during travel'
Save-Eyes 'car-moving-consumable-wheel'
Touch-MountedItem $slot 0
$a=Get-Addon
Assert-Equal $a.nativeGrabHand 1 'Touched consumable uses native right hand'
Assert-Equal $a.nativeGrabBase 0x32C75 'Selected drink held without consuming'
Park-Hands
Open-MountedWheel r
Assert-Equal (Get-Addon).openHand 1 'Wheel can reopen while holding consumable'
Send-XR @('btn b press 150');Start-Sleep -Milliseconds 400
Assert-Equal (Get-Addon).openHand -1 'B closes wheel'
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 3 'Closing selector does not exit car'
Send-XR @('stick l 0 -1')
$deadline=[DateTime]::UtcNow.AddSeconds(8)
do{Start-Sleep -Milliseconds 200;$gear=Get-Native ('Call (CompileScript "FCVehicleReverse.txt") (StringToRef "'+$id+'")')}while($gear -ne 1 -and [DateTime]::UtcNow -lt $deadline)
Assert-Equal $gear 1 'Left stick backward brakes then reverses'
Send-XR @('stick l 0 0');Start-Sleep -Seconds 1
Assert-Equal (Get-Native 'MenuMode') 0 'No accidental pause while using either wheel'
Send-XR @('btn b press 150');Start-Sleep -Seconds 2
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 0 'B exits car once selector is closed'
