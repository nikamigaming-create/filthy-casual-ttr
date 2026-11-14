. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('head to 0 1.1 0 0 -35 0 600','hand r to grip 0.12 0.90 -0.35 0 -60 180 700')
Start-Sleep -Seconds 4
Save-Eyes 'cuff-final-auto'
Send-Game @('sc_reload')|Out-Null
Start-Sleep -Seconds 4
Save-Eyes 'cuff-final-manual'
Send-Game @('sc_reload')|Out-Null
Assert-Equal (Get-Addon).manualReload $false 'Final cuff comparison restores automatic reload'
