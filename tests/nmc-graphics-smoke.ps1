. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 15
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 700}
Send-XR @('input clear','head to 0 1.1 0 0 0 0 500')
Park-Hands
Start-Sleep -Seconds 4
Assert-Equal ((Get-Addon).menu) $false 'Texture smoke fixture reached gameplay'
Save-Eyes 'nmc-graphics-interior'
Send-XR @('head to 0 1.1 0 60 -15 0 650')
Start-Sleep -Seconds 4
Save-Eyes 'nmc-graphics-interior-side'
Write-Output 'PASS: NMC-enabled profile loaded the disposable fixture and produced paired eye captures'
