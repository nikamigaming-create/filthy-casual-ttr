. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('click l down','click r down')
Start-Sleep -Milliseconds 750
Send-XR @('click l up','click r up')
Start-Sleep -Milliseconds 200
Assert-Equal (Get-Addon).heightMenu $true 'Final height panel opens'
Assert-Equal (Get-Addon).popupVisible $true 'Quiet mode preserves final height panel'
Save-Eyes 'cap-height-panel-final'
Send-XR @('btn y press 180')
Start-Sleep -Milliseconds 250
Assert-Equal (Get-Addon).selectorPopups $true 'Final Y toggle enables messages'
Save-Eyes 'cap-height-panel-on-final'
Send-XR @('btn y press 180')
Start-Sleep -Milliseconds 250
Assert-Equal (Get-Addon).selectorPopups $false 'Final Y toggle restores quiet mode'
Send-XR @('btn b press 180')
Start-Sleep -Milliseconds 200
Assert-Equal (Get-Addon).popupVisible $true 'Final height save confirmation remains visible'
