. "$PSScriptRoot\runtime-helpers.ps1"
function Controls {Send-Game @('sc_controls_state')|Out-Null;Get-Content "$script:Session\controls-state.json" -Raw|ConvertFrom-Json}
function Mode-Check([string]$name,[string]$capture){Assert-Equal (Controls).liveMode $name 'Active provider mode matches gameplay';Send-Game @('sc_controls')|Out-Null;Start-Sleep -Milliseconds 500;Assert-Equal (Controls).mode $name 'Editor follows current mode';Save-Eyes $capture;Send-XR @('btn b press 120');Start-Sleep -Milliseconds 350}
& "$PSScriptRoot\controls-runtime.ps1"
Assert-Equal (Controls).modeCount 3 'Installed vehicle provider advertises modes while on foot'
Send-XR @('input clear','head pose 0 1.2 0 0 0 0')
Send-Game @('sc_controls')|Out-Null;Start-Sleep -Milliseconds 400
# Actual mode button: MODE >. It configures cars and aircraft while on foot.
Send-XR @('hand r aimtrim 0 0','hand r grip pose -.43 1.373 -.45 0 0 0');Start-Sleep -Milliseconds 300
Assert-Equal (Controls).pointerRow 36 'Ray intersects next-mode button'
Send-XR @('trigger r 1');Start-Sleep -Milliseconds 100;Send-XR @('trigger r 0');Start-Sleep -Milliseconds 350
Assert-Equal (Controls).mode 'vehicles.driving' 'Driving bindings available without entering car'
Save-Eyes 'robco-driving-offline'
Send-XR @('trigger r 1');Start-Sleep -Milliseconds 100;Send-XR @('trigger r 0');Start-Sleep -Milliseconds 350
Assert-Equal (Controls).mode 'vehicles.flight' 'Flight bindings available without entering aircraft'
Save-Eyes 'robco-flight-offline'
Send-XR @('btn b press 120');Start-Sleep -Milliseconds 350
Assert-Equal (Controls).mode 'onfoot' 'Closing inactive-mode editing restores gameplay mode'
Park-Hands
Send-Game @('sc_settings')|Out-Null;Start-Sleep -Milliseconds 500;Save-Eyes 'robco-height-filled';Send-XR @('btn b press 120');Start-Sleep -Milliseconds 350
Send-Game @('player.AddItem 0000434F 1','player.AddItem 00004353 1','sc_weapons')|Out-Null;Start-Sleep -Milliseconds 500;Save-Eyes 'robco-weapons-filled';Send-XR @('btn b press 120');Start-Sleep -Milliseconds 350
Send-Game @('player.AddItem 00015169 5','sc_aid')|Out-Null;Start-Sleep -Milliseconds 500;Save-Eyes 'robco-aid-filled';Send-XR @('btn b press 120');Start-Sleep -Milliseconds 350
& "$PSScriptRoot\vehicle-drive.ps1"
& "$PSScriptRoot\vertibird-smoke.ps1"
Assert-Equal (Controls).liveMode 'onfoot' 'Landing restores on-foot profile'
Write-Output 'PASS: populated RobCo overlays, editor, installed provider catalog, vehicle mode routing, roadster drive, Vertibird flight and landing'
