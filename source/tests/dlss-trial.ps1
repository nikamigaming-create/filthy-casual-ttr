. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 8
for($notice=0;$notice -lt 3 -and (Get-Addon).nativeTopMenu -eq 1001;$notice++){
 Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600
}
Park-Hands
Start-Sleep -Seconds 12
Get-Addon | Select-Object pid,nativeTopMenu | ConvertTo-Json -Compress
Save-Eyes 'dlss-trial'
Get-Process ttwvr_dlss_host -ErrorAction SilentlyContinue | Select-Object Id,Path
Start-Sleep -Seconds 3
