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
foreach($mode in 'native-sync','no-sync','all-controls'){
 if($mode -eq 'no-sync'){Send-Game @('player.SyncPos')|Out-Null}
 if($mode -eq 'all-controls'){Send-Game @('EnablePlayerControls')|Out-Null}
 Send-XR @('head pose 0 1.1 0 0 0 0','hand l grip pose -.25 .8 -.3 0 40 0','hand r grip pose .25 .8 -.3 0 40 0','grip l 0','grip r 0');Start-Sleep -Milliseconds 400
 $before=Get-Addon
 Send-XR @('hand l grip pose -.25 1.0 -.55 0 40 0');Start-Sleep -Milliseconds 500
 $after=Get-Addon
 $results+=@{mode=$mode;before=$before.palms[0];after=$after.palms[0];beforeNative=$before.nativeGripTracking[0];afterNative=$after.nativeGripTracking[0];freePose=$after.freePoseWeight;nativeGrab=$after.nativeGrabRef}
 Save-Eyes ('vehicle-pose-'+$mode)
}
$results|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\work\vehicles\pose-probe.json"
$results|ConvertTo-Json -Depth 6|Write-Output
Get-Native ($fn+'2')|Out-Null
