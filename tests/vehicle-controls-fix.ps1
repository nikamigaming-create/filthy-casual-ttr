. "$PSScriptRoot\runtime-helpers.ps1"
. "$PSScriptRoot\vehicle-drive.ps1"
# Enter the same native car again for gamepad steering without gripping.
Get-Native ($fn+'1')|Out-Null;Start-Sleep -Seconds 3
Park-Hands
Send-XR @('input clear','stick l .55 0','trigger r .4');Start-Sleep -Seconds 2
$w=Wheel-State
Assert-Equal ($w.steer -gt .45) $true 'Gamepad left-stick steering without a wheel grip'
Assert-Equal ($w.gas -gt .3) $true 'Independent trigger throttle without gripping'
Assert-Equal ($w.grabbed[0] -eq 0 -and $w.grabbed[1] -eq 0) $true 'Both hands free in gamepad driving'
Send-XR @('stick l 0 0','trigger r 0','trigger l 1');Start-Sleep -Seconds 2
Send-XR @('trigger l 0','btn b press 180');Start-Sleep -Seconds 2
Assert-Equal (Get-Native 'player.GetDead') 0 'Driver alive after physical and gamepad drive'
Write-Output 'PASS driving controls and grip attachment'
