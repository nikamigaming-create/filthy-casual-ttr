. "$PSScriptRoot/equipment-device-input.ps1"

function Check-General([bool]$worn) {
    $view = Page
    Assert-Equal ([bool]$view.onArm) $worn 'General is on the expected device'
    Assert-Equal $view.title 'GENERAL SETTINGS' 'FILTHY opens directly to General'
    Assert-Equal $view.availablePages 2 'General is the only FILTHY section'
    Choose 'Wrist flashlight'
    Assert-Equal (Page).title 'WRIST FLASHLIGHT' 'Flashlight controls open'
    Button b
    Assert-Equal (Page).title 'GENERAL SETTINGS' 'Back returns to General'
}
Send-XR @('input clear','profile touch','head valid on','btn a press 170')
Send-Game 'load SC_QA_fixture' | Out-Null
Start-Sleep -Seconds 7
Send-Game @('CloseAllMenus','player.SetWeaponOut 0') | Out-Null
Send-XR @('head to 0 1.11 0 8 -29 0 560','hand l to grip -.07 .97 -.45 -12 47 152 650',
          'hand r to grip -.045 .96 -.28 0 -15 0 650')
Start-Sleep -Milliseconds 950
Send-XR 'trigger r .99'
Start-Sleep -Milliseconds 300
for ($i=0; $i -lt 6; $i++) { Grip r; if ((Page).onArm -and (Page).title -eq 'GENERAL SETTINGS') { break } }
Send-XR 'trigger r 0'
Start-Sleep -Milliseconds 350
Check-General $true
Grip r
Assert-Equal (Page).title 'GENERAL SETTINGS' 'Grip keeps General selected'
Assert-Equal (Get-Native 'IsModLoaded "RobCo Eyebot.esp"') 0 'The recorder plugin is absent'
Send-XR 'trigger l .96'
Start-Sleep -Milliseconds 190
Send-XR 'trigger l 0'
Start-Sleep -Milliseconds 850
Assert-Equal (Get-Native 'GetActiveMenuMode') 1002 'Full Pip-Boy opens to Items'
Send-XR 'hand r to grip .22 .80 -.28 0 -50 0 720'
Start-Sleep -Milliseconds 950
for ($i=0; $i -lt 5 -and (Get-Native 'GetActiveMenuMode') -ne 1005; $i++) { Category }
Check-General $false
Save-Eyes 'rc22-general-only-full' | Out-Host
Button x
Assert-Equal (Page).title 'GENERAL SETTINGS' 'Full-size page cycle keeps General selected'
Category l
Assert-Equal (Get-Native 'GetActiveMenuMode') 1023 'Stock Data returns after leaving FILTHY'
