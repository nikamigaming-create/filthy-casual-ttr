. "$PSScriptRoot\casual-guide-helpers.ps1"
Load-Guide
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
Smooth-Touch 1
Rest-Hands
Send-XR @('btn a down')
Start-Sleep -Milliseconds 1400
Send-XR @('btn a up')
Start-Sleep -Milliseconds 800
Send-XR @('hand l to grip -0.4 0.55 -0.08 0 30 0 1200','hand r to grip 0.4 0.70 -0.22 0 55 0 1200','head to 0 1.1 0 0 -55 0 1500')
Start-Sleep -Seconds 2
$a=Get-Addon
$a | ConvertTo-Json -Depth 8 | Set-Content "$script:Repo\evidence\pouch-probe.json"
$a.pouch | ConvertTo-Json -Depth 5 | Write-Output
Save-Eyes 'seated-pouch-clear'
Send-Game @('player.SetCurrentAmmoRounds 1') | Out-Null
Send-XR @('trigger r pull 180')
Start-Sleep -Seconds 3
Write-Output "Manual mode ammo after shot: $(Get-Native 'player.GetCurrentAmmoRounds')"
Send-XR @('btn a down')
Start-Sleep -Milliseconds 1400
Send-XR @('btn a up')
Start-Sleep -Seconds 1
Assert-Equal (Get-Addon).manualReload $false 'Pouch probe restores automatic mode'
