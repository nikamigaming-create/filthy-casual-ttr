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
Send-XR @('hand l grip pose -.14 .88 -.41 0 10 0','hand r grip pose .15 .88 -.41 0 10 0')
Start-Sleep -Seconds 2
Send-XR @('grip l .9','grip r .9');Start-Sleep -Seconds 1
Write-Output (Get-Content "$script:Session\vehicle-state.json" -Raw)
Save-Eyes 'vehicle-direct-grip-probe'
