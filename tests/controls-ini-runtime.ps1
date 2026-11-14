. "$PSScriptRoot\runtime-helpers.ps1"
function Controls {Send-Game @('sc_controls_state')|Out-Null;Get-Content "$script:Session\controls-state.json" -Raw|ConvertFrom-Json}
Send-XR @('capture off','pace free 60','input clear','head height 1.2')
Start-Sleep -Milliseconds 600
Assert-Equal (Controls).visible $true 'INI startup fallback opens editor'
Send-XR @('btn b press 120');Start-Sleep -Milliseconds 350
Assert-Equal (Controls).visible $false 'Startup editor closes normally'
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 120');Start-Sleep -Milliseconds 400}
Park-Hands
Assert-Equal (Controls).bindings[10][0] 9 'Typed Menu maps to right-stick click'
Send-XR @('click r down');Start-Sleep -Milliseconds 1400;Send-XR @('click r up');Start-Sleep -Milliseconds 400
Assert-Equal (Get-Addon).heightMenu $true 'Replacement Menu hold opens settings'
Send-XR @('btn b press 120');Start-Sleep -Milliseconds 350
Send-XR @('grip l 1','grip r 1');Start-Sleep -Milliseconds 1100;Send-XR @('grip l 0','grip r 0');Start-Sleep -Milliseconds 400
Assert-Equal (Controls).visible $true 'Typed grip combo opens editor without Menu'
Assert-Equal (Controls).modelsReady $true 'Controller assets available in live editor'
Save-Eyes 'controls-typed-menu'
Send-XR @('btn b press 120');Start-Sleep -Milliseconds 350
Assert-Equal (Controls).visible $false 'Editor releases gameplay input after close'
Send-XR @('btn y press 120');Start-Sleep -Milliseconds 350
Assert-Equal ((Get-Addon).menu -ne 0) $true 'Unchanged Y still pauses'
Send-XR @('btn b press 120');Start-Sleep -Milliseconds 350
Assert-Equal (Get-Addon).menu 0 'Unchanged B still returns to play'
Write-Output 'PASS: INI replacement Menu, startup access, direct combo, native Pause/Back'
