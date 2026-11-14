. "$PSScriptRoot\casual-guide-helpers.ps1"
Load-Guide
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
Touch-Slot 1 0 | Out-Null
Start-Sleep -Milliseconds 1200
Assert-Equal (Get-Addon).weaponHand 1 'Selected gun starts with right-hand convenience hold'
Assert-Equal (Get-Addon).manualReloadKnown $true 'Native reload hooks and live setting are available'
Assert-Equal (Get-Addon).manualReload $false 'User starts in automatic reload mode'
Send-Game @('player.SetCurrentAmmoRounds 5') | Out-Null
Send-XR @('btn a press 180')
Start-Sleep -Seconds 2
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'Short A still reloads through native input'
Send-Game @('player.SetCurrentAmmoRounds 5') | Out-Null
Send-XR @('btn a down')
Start-Sleep -Milliseconds 2600
Assert-Equal (Get-Addon).manualReload $true 'Long A changes live native manual reload once per hold'
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 5 'Long A does not also send a short native reload'
Send-XR @('btn a up')
Start-Sleep -Milliseconds 400
Assert-Equal (Get-Addon).seatedPouchApplied $true 'Manual mode uses native seated pouch placement'
Send-XR @('head to 0 1.1 0 0 -55 0 1000')
Start-Sleep -Milliseconds 1200
Save-Eyes 'seated-ammo-pouch'
Send-XR @('head to 0 1.1 0 20 -55 0 1000')
Start-Sleep -Milliseconds 1200
Save-Eyes 'seated-ammo-pouch-look'
Send-XR @('head to 0 1.1 0 0 -3 0 1000')
Start-Sleep -Milliseconds 1200
Send-XR @('btn a down')
Start-Sleep -Milliseconds 1400
Send-XR @('btn a up')
Start-Sleep -Milliseconds 400
Assert-Equal (Get-Addon).manualReload $false 'Second long A restores automatic reload'
Assert-Equal (Get-Addon).seatedPouchApplied $false 'Automatic mode restores the native pouch position'
Assert-Equal (Get-Native 'MenuMode') 0 'Reload mode switch stays in gameplay'
Send-XR @('grip r 1')
Start-Sleep -Milliseconds 350
Assert-Equal (Get-Addon).weaponHand -1 'Deliberate gun squeeze returns grip ownership to TTWVR'
Send-XR @('grip r 0')
Start-Sleep -Milliseconds 500
Write-Output "Native weapon state after grip release: $(Get-Native 'player.IsWeaponOut')"
Send-Game @('player.SetWeaponOut 0') | Out-Null
Start-Sleep -Milliseconds 1400
Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Addon does not redraw a gun after native grip takeover'
Send-Game @('player.AddItem 0001519B 2') | Out-Null
Choose-Aid 86427
Assert-Equal (Get-Addon).aidGripLatch $true 'Food starts with convenience hold'
Send-XR @('grip r 1')
Start-Sleep -Milliseconds 350
Assert-Equal (Get-Addon).aidGripLatch $false 'Food squeeze hands grip back to TTWVR'
Send-XR @('grip r 0')
Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).nativeGrabRef 0 'Native food release drops the held item'
Choose-Aid 86377
Assert-Equal (Get-Addon).nativeStimHand 1 'Stimpak starts in native right hand'
Send-XR @('grip r 1')
Start-Sleep -Milliseconds 350
Assert-Equal (Get-Addon).aidGripLatch $false 'Stimpak squeeze hands grip back to TTWVR'
Send-XR @('grip r 0')
Start-Sleep -Milliseconds 900
Write-Output "Native Stimpak hand after grip release: $((Get-Addon).nativeStimHand)"
Save-Eyes 'native-grip-final'
