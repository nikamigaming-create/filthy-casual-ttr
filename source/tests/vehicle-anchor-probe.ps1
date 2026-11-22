. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -10 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('tdetect','player.SetWeaponOut 0','set GameHour to 12','player.SetPos X -72500','player.SetPos Y 2000','player.SetAngle Z 0')|Out-Null;Start-Sleep -Seconds 1
$car=Get-Native 'player.PlaceAtMe (GetFormFromMod "UmDrivingVIIIb.esp" "00E6BC") 1 300 0'
Assert-Equal ($car -ne 0) $true 'Native roadster spawned from original car mod'
$id=('{0:X8}' -f $car)
$fn='Call (CompileScript "FCVehicleTest.txt") "'+$id+'" '
Get-Native ($fn+'4')|Write-Output
Get-Native ($fn+'5')|Write-Output
Get-Native ($fn+'3')|Write-Output
Start-Sleep -Seconds 1
$a=Get-Addon
$rx=$a.nativeGripPoints[1][0]-$a.nativeGripPoints[0][0];$ry=$a.nativeGripPoints[1][1]-$a.nativeGripPoints[0][1]
$heading=[Math]::Atan2(-$ry,$rx)*180/[Math]::PI
Get-Native ('Call (CompileScript "FCVehicleAlign.txt") "'+$id+'" '+$heading.ToString('F3',[Globalization.CultureInfo]::InvariantCulture))|Write-Output
Start-Sleep -Milliseconds 500
Get-Native ($fn+'1')|Write-Output;Start-Sleep -Seconds 1
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 3 'Native vehicle driving state entered'

Send-XR @('trigger r .5');Start-Sleep -Seconds 6
Send-XR @('trigger r 0','trigger l 1');Start-Sleep -Seconds 2
Send-XR @('trigger l 0','btn b press 200');Start-Sleep -Seconds 2
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 0 'Native exit completes'
