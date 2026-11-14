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

function Probe([string]$label){
 $a=Get-Addon
 Send-Game @('sc_controls_state')|Out-Null
 Copy-Item "$script:Session\controls-state.json" "$script:Repo\work\vehicle-controls\reach-$label-input.json" -Force
 Write-Output "$label menu=$($a.menu) top=$($a.nativeTopMenu) controls=$($a.heightMenu)"
 Save-Eyes "reach-$label"
}
Probe 'parked'
Send-XR @('hand r to grip .15 .88 -.41 0 10 0 800');Start-Sleep -Milliseconds 1000
Probe 'right-moved'
Send-XR @('hand l to grip -.14 .88 -.41 0 10 0 800');Start-Sleep -Milliseconds 1000
Probe 'left-moved'
Send-XR @('grip r .9');Start-Sleep -Milliseconds 500
Probe 'right-grip'
Send-XR @('grip l .9');Start-Sleep -Milliseconds 500
Probe 'left-grip'

Send-XR @('grip r 0','grip l 0','touch l index on','touch r index on','thumbrest l on','thumbrest r on');Start-Sleep -Milliseconds 600
Probe 'touches'
Send-XR @('hand l grip pose -.14 .88 -.41 0 10 0','hand r grip pose .15 .88 -.41 0 10 0');Start-Sleep -Milliseconds 800
Probe 'fast-pose'
Send-XR @('grip l .9','grip r .9');Start-Sleep -Milliseconds 600
Probe 'both-touch-grip'
