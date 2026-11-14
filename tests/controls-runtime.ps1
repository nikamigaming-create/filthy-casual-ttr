. "$PSScriptRoot\runtime-helpers.ps1"
function Controls {Send-Game @('sc_controls_state')|Out-Null;Get-Content "$script:Session\controls-state.json" -Raw|ConvertFrom-Json}
function Button([string]$name){Send-XR @("btn $name press 100");Start-Sleep -Milliseconds 280}
function Row([int]$target){for($n=0;$n -lt 30;$n++){$c=Controls;if($c.selected -eq $target){return};Send-XR @('stick r 0 -1');Start-Sleep -Milliseconds 110;Send-XR @('stick r 0 0');Start-Sleep -Milliseconds 120};throw 'Could not select row'}
Send-XR @('capture off','pace free 60','input clear','head height 1.2','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 5
if((Get-Addon).menu){Button a}
Park-Hands
Send-XR @('head to 0 1.2 0 0 0 0 300','hand l to grip -.28 .9 -.4 0 0 0 300','hand r to grip .28 .9 -.4 0 0 0 300')
Start-Sleep -Milliseconds 600
Button y
Assert-Equal ((Get-Addon).menu -ne 0) $true 'Native Y still opens Pause'
Button b
Assert-Equal (Get-Addon).menu 0 'Native B still closes Pause'
Send-XR @('btn menu down');Start-Sleep -Milliseconds 1200;Send-XR @('btn menu up');Start-Sleep -Milliseconds 300
Assert-Equal (Get-Addon).heightMenu $true 'Existing Menu hold still opens height settings'
Send-XR @('click l press 100');Start-Sleep -Milliseconds 300
Assert-Equal (Controls).visible $true 'Left stick click in settings opens controller editor'
Save-Eyes 'controls-native-defaults'|Out-Host
# Point the real XR aim ray at the first visible row; physical trigger selects.
Send-XR @('hand r aimtrim 0 0','hand r grip pose .35 1.45 -.45 0 0 0');Start-Sleep -Milliseconds 400
Assert-Equal (Controls).pointerRow 0 'Right controller ray hits first control row'
Send-XR @('trigger r 1');Start-Sleep -Milliseconds 100;Send-XR @('trigger r 0');Start-Sleep -Milliseconds 250
Assert-Equal (Controls).phase 2 'Pointer trigger begins capture on the selected row'
Button y
Assert-Equal (Controls).phase 3 'Pointer-started capture records a face button'
Button b
Assert-Equal (Controls).bindings[0][0] 0 'Cancel leaves native binding unchanged'
Send-XR @('hand r grip pose .35 .6 -.45 0 0 0');Start-Sleep -Milliseconds 200

Assert-Equal (Controls).modelsReady $true 'Actual Quest Touch Plus 3D models rendered'
Row 5
Button a
Assert-Equal (Controls).phase 2 'Editor waits for a new physical input'
Button x
Assert-Equal (Controls).phase 3 'X captured for right trigger with confirmation'
Save-Eyes 'controls-binding-confirm'|Out-Host
Button a
Assert-Equal (Controls).bindings[5][0] 2 'Right trigger remapped to X'
# Save/load through real editor controls; isolated simulator profiles only.
Row 26
Button a
Assert-Equal (Test-Path "$script:Session\controls-profiles\profile-1.fcp") $true 'Profile saved from in-game panel'
Row 28
Button a
Assert-Equal (Controls).bindings[5][0] 5 'Restore all returns right trigger to native source'
Row 27
Button a
Assert-Equal (Controls).bindings[5][0] 2 'Saved profile restores X binding'
Button b
Assert-Equal (Controls).visible $false 'B exits editor'
Send-XR @('btn x down');Start-Sleep -Milliseconds 220
$c=Controls;$q=@($c.queried|Where-Object {$_.name -eq 'trigger' -and $_.hand -eq 1})[0]
Assert-Equal $q.value 1 'Real native right trigger query receives remapped X'
Send-XR @('btn x up');Start-Sleep -Milliseconds 220
$c=Controls;$q=@($c.queried|Where-Object {$_.name -eq 'trigger' -and $_.hand -eq 1})[0]
Assert-Equal $q.value 0 'Remapped trigger releases correctly'
# Fixed physical recovery gesture works independently of configured bindings.
Send-XR @('btn menu down','click r down');Start-Sleep -Milliseconds 1300;Send-XR @('btn menu up','click r up');Start-Sleep -Milliseconds 350
Assert-Equal (Controls).visible $true 'Fixed recovery gesture reopens editor'
Row 5
Save-Eyes 'controls-remapped-right-trigger'|Out-Host
Send-XR @('grip r 1','trigger r 1','thumbrest r on','touch r index on');Start-Sleep -Milliseconds 700
$a=Get-Addon
Assert-Equal ($a.fingerGestures[1].index -gt .95) $true 'Physical fingers still curl while virtual trigger is suppressed'
Save-Eyes 'controls-physical-fingers'|Out-Host
Send-XR @('input clear');Start-Sleep -Milliseconds 350
Row 28
Button a
Row 26
Button a
Button b
Send-XR @('state visible');Start-Sleep -Milliseconds 300;Send-XR @('state focused');Start-Sleep -Milliseconds 400
Button y
Assert-Equal ((Get-Addon).menu -ne 0) $true 'Restored profile preserves Pause after focus cycle'
Button b
Assert-Equal (Get-Addon).menu 0 'Restored profile preserves Back after focus cycle'
Write-Output 'PASS: live editor, Quest panel, remapping, save/load, defaults, recovery, physical fingers, native controls'
