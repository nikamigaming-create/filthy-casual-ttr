. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
Touch-Slot 1 0 | Out-Null
Start-Sleep -Milliseconds 1200
Assert-Equal (Get-Native 'player.IsWeaponOut') 1 'Selected pistol drawn'
Send-Game @('player.SetCurrentAmmoRounds 5') | Out-Null
Send-XR @('btn a press 180')
Start-Sleep -Seconds 2
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'A performs native reload before empty'
Park-Hands
Send-XR @('btn y press 180')
Start-Sleep -Milliseconds 700
Assert-Equal (Get-Native 'MenuMode') 1 'Native pause menu opens'
Send-Game @('CloseAllMenus') | Out-Null
Start-Sleep -Milliseconds 1400
Assert-Equal (Get-Native 'MenuMode') 0 'Menu closed'
Assert-Equal (Get-Native 'player.IsWeaponOut') 1 'Selected weapon returns after native menu teardown'
Assert-Equal (Get-Native 'player.GetEquippedObject 5') 17231 'Menu return keeps same selected weapon'
Send-Game @('DisablePlayerControls 0 1 0 0 0 0 0','player.SetWeaponOut 0') | Out-Null
Start-Sleep -Milliseconds 1300
Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Disabled combat controls prevent forced draw'
Send-Game @('EnablePlayerControls 0 1 0 0 0 0 0') | Out-Null
Start-Sleep -Milliseconds 1300
Assert-Equal (Get-Native 'player.IsWeaponOut') 1 'Same selection resumes after combat controls return'
Send-XR @('hand r grip pose 0.25 1.1 -0.4 0 0 0')
Save-Eyes 'weapon-resume-final'
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
Touch-Slot 6 1 | Out-Null
Start-Sleep -Milliseconds 1400
Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Center cap stays empty without automatic redraw'
Send-Game @('CloseAllMenus') | Out-Null
Start-Sleep -Milliseconds 1300
Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Menu teardown does not undo explicit empty selection'
