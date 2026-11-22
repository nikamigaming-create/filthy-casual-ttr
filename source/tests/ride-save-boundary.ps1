. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','profile touch','head pose 0 1.1 0 0 -15 0','btn a press 180')
Send-Game @('load FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('tgm','player.SetWeaponOut 0','player.SetPosEx -56000 -79000 6000')|Out-Null;Start-Sleep -Seconds 3
$n=Get-Native 'player.PlaceAtMe VCrFarmBigHorner 1';$id=('{0:X8}' -f [int32]$n)
Send-Game @("$id.SetRestrained 1","player.MoveTo $id 85 0 0",'DisablePlayerControls 1 0 0 0 0 0 0')|Out-Null;Start-Sleep -Seconds 1
Send-Game @('save FC_QA_Ride_Boundary_20261113')|Out-Null;Start-Sleep -Seconds 2
Send-Game @("$id.SetRestrained 0",'EnablePlayerControls 1 0 0 0 0 0 0','load FC_QA_Ride_Boundary_20261113')|Out-Null;Start-Sleep -Seconds 6
Write-Output "Saved movement lock after load: $(Get-Native 'GetPlayerControlsDisabled 1')"
Get-Content "$script:Session\bighorner-state.json" -Raw
Send-Game @("$id.SetRestrained 0",'EnablePlayerControls 1 0 0 0 0 0 0')|Out-Null
