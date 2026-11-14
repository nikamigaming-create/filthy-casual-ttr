. "$PSScriptRoot\casual-guide-helpers.ps1"
Load-Guide
Assert-Equal (Get-Addon).heightOffset 0 'Capture begins at uncalibrated seated height'
Start-Take 'height-weapons' 110
try {
    Mark 'Seated before';Start-Sleep -Seconds 3
    Send-XR @('head to 0 1.1 0 0 -50 0 1800');Start-Sleep -Seconds 6
    Save-Eyes 'casual-height-before'
    Mark 'Height controls';Send-XR @('click l down','click r down');Start-Sleep -Milliseconds 750
    Send-XR @('click l up','click r up');Start-Sleep -Seconds 4
    Mark 'Calibrate';Send-XR @('btn a press 180');Start-Sleep -Seconds 4
    Assert-Equal (Get-Addon).heightOffset 0.55 'Seated calibration raises eye height 55 cm'
    Mark 'Save height';Send-XR @('btn b press 180');Start-Sleep -Seconds 4
    Save-Eyes 'casual-height-after'
    Send-XR @('head to 0 1.1 0 0 -3 0 1900');Start-Sleep -Seconds 3
    Mark 'Open weapons';Rest-Hands;Send-XR @('click r press 180');Start-Sleep -Seconds 5
    Mark 'Touch pistol';Smooth-Touch 1
    Assert-Equal (Get-Native 'player.GetEquippedObject 5') 17231 'Guide selects intended pistol'
    Aim-Item;Start-Sleep -Seconds 4
    Save-Eyes 'casual-pistol'
    Send-Game @('player.SetCurrentAmmoRounds 1') | Out-Null
    Mark 'Shoot and auto reload';Send-XR @('trigger r pull 180');Start-Sleep -Seconds 4
    Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'Native empty-magazine auto reload'
    Send-XR @('trigger r pull 180');Start-Sleep -Seconds 3
    Mark 'A reload';Send-XR @('btn a press 180');Start-Sleep -Seconds 4
    Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'A reload works before empty'
    Mark 'Center cap';Rest-Hands;Send-XR @('click r press 180');Start-Sleep -Seconds 4
    Smooth-Touch 6;Rest-Hands;Start-Sleep -Seconds 4
    Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Center cap leaves hands empty'
    Mark 'Run';Send-XR @('stick l 0 1');Start-Sleep -Milliseconds 450
    Assert-Equal (Get-Native 'player.IsRunning') 1 'Full stick runs'
    Send-XR @('stick l center');Start-Sleep -Seconds 4
    Finish-Take
} finally {Stop-Take}
