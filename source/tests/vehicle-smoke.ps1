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
Save-Eyes 'vehicle-roadster-outside'
Get-Native ($fn+'1')|Write-Output;Start-Sleep -Seconds 3
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 3 'Native vehicle driving state entered'
Save-Eyes 'vehicle-roadster-seat'
Send-XR @('head to .08 1.1 0 25 -20 0 1200');Start-Sleep -Milliseconds 1400
Save-Eyes 'vehicle-roadster-look'
$state=Get-Addon;$state|ConvertTo-Json -Depth 14|Set-Content "$script:Repo\work\vehicles\baseline-state.json"
Get-Native ($fn+'2')|Write-Output;Start-Sleep -Seconds 2
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 0 'Native vehicle exit restores on-foot mode'
Save-Eyes 'vehicle-roadster-exit'
