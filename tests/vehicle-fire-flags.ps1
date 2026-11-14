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

if(Get-Command Mode-Check -ErrorAction SilentlyContinue){Mode-Check 'vehicles.driving' 'robco-driving-active'}
function Wheel-State { Get-Content "$script:Session\vehicle-state.json" -Raw|ConvertFrom-Json }

function Shot([string]$Label){
 for($i=0;$i -lt 3 -and (Get-Native 'MenuMode');$i++){Send-XR @('btn a press 150');Start-Sleep -Milliseconds 500}
 Write-Output "SHOT STATE $Label menu=$(Get-Native 'MenuMode') drawn=$(Get-Native 'player.IsWeaponOut') hand=$((Get-Addon).weaponHand)"

 Send-Game @('player.SetWeaponOut 1')|Out-Null;Start-Sleep -Seconds 2
 Write-Output "Native sit state=$((Get-Content "$script:Session\vehicle-state.json" -Raw|ConvertFrom-Json).actorState) drawn=$(Get-Native 'player.IsWeaponOut')"
 $before=Get-Native 'player.GetCurrentAmmoRounds'
 Send-XR @('trigger r 1');Start-Sleep -Milliseconds 700
 Send-Game @('sc_controls_state')|Out-Null
 Copy-Item "$script:Session\controls-state.json" "$script:Repo\work\vehicle-controls\$Label-input.json" -Force
 Save-Eyes "vehicle-fire-$Label"
 Send-XR @('trigger r 0');Start-Sleep -Milliseconds 300
 $after=Get-Native 'player.GetCurrentAmmoRounds'
 Write-Output "SHOT $Label before=$before after=$after"
}
for($i=0;$i -lt 3 -and (Get-Native 'MenuMode');$i++){Send-XR @('btn a press 150');Start-Sleep -Milliseconds 500}
Send-Game @('player.AddItem 0000434F 1','player.AddItem 00004241 40','sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 0000434F')|Out-Null;Start-Sleep -Seconds 2
Send-XR @('hand r grip pose .22 .95 -.43 0 55 0','hand l grip pose -.3 .75 -.23 0 55 0')
Shot 'baseline'
foreach($bit in 2,3,5,6){
 Send-Game @('DisablePlayerControls 1 0 1 1 0 1 1')|Out-Null
 $args=@(0,0,0,0,0,0,0);$args[$bit]=1
 Send-Game @('EnablePlayerControls '+($args -join ' '))|Out-Null
 Shot ("flag-"+$bit)
 Write-Output (Get-Content "$script:Session\vehicle-state.json" -Raw)
}
