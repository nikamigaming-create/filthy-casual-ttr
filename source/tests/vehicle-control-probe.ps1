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


$results=@()
foreach($flag in 0,1,2,3,4,5,6,7){
 Send-Game @('DisablePlayerControls 1 0 1 1 0 1 1')|Out-Null
 $args=@(0,0,0,0,0,0,0);if($flag -eq 7){$args=@(1,1,1,1,1,1,1)}else{$args[$flag]=1}
 Send-Game @('EnablePlayerControls '+($args -join ' '))|Out-Null
 Send-XR @('hand l grip pose -.3 .7 -.3 0 0 0');Start-Sleep -Milliseconds 200
 $before=Get-Addon
 Send-XR @('hand l grip pose -.3 1.0 -.3 0 0 0');Start-Sleep -Milliseconds 300
 $after=Get-Addon
 $results+=@{flag=$flag;before=$before.fingerGestures[0].applied;after=$after.fingerGestures[0].applied;beforePalm=$before.palms[0];afterPalm=$after.palms[0]}
}
$results|ConvertTo-Json -Depth 5|Set-Content "$script:Repo\work\vehicles\control-gate-probe.json"
$results|ConvertTo-Json -Depth 5|Write-Output
Save-Eyes 'vehicle-control-gate'
Get-Native ($fn+'2')|Out-Null
