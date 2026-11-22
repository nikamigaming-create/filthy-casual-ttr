. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -20 0','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 500}
Park-Hands;Send-Game @('sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 0000434F')|Out-Null;Start-Sleep -Seconds 1
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null}
Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0');Start-Sleep -Milliseconds 700
Send-XR @('btn a press 180');Start-Sleep -Seconds 1
$a=Get-Addon;$a.physicalParts|ConvertTo-Json -Depth 7
Start-Sleep -Seconds 3
$a=Get-Addon;$a.physicalParts|ConvertTo-Json -Depth 7
$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-physics-probe-state.json"
Save-Eyes 'magazine-physics-probe'
