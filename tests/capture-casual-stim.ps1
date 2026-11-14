. "$PSScriptRoot\casual-guide-helpers.ps1"
Load-Guide
Send-Game @('player.damageav health 50') | Out-Null
Choose-Aid 86377
$doses=Get-Native 'player.GetItemCount Stimpak'
Aim-Item
Start-Take 'stimpak' 55
try {
    Mark 'Trigger then jab';Start-Sleep -Seconds 5
    Send-XR @('head to 0 1.1 0 0 -35 0 1600');Start-Sleep -Seconds 2
    Send-XR @('hand r to grip 0.10 0.67 -0.50 0 0 0 1500');Start-Sleep -Seconds 2
    Send-XR @('trigger r 1');Start-Sleep -Milliseconds 500
    Send-XR @('hand r to grip 0.02 0.67 0.03 0 0 0 500');Start-Sleep -Seconds 3
    Send-XR @('trigger r 0')
    Assert-Equal (Get-Native 'player.GetItemCount Stimpak') ($doses-1) 'Recorded seated body jab uses one native dose'
    Rest-Hands;Start-Sleep -Seconds 3
    Send-XR @('head to 0 1.1 0 0 -3 0 1700');Start-Sleep -Seconds 2
    Mark 'Quiet height panel';Send-XR @('click l down','click r down');Start-Sleep -Milliseconds 750
    Send-XR @('click l up','click r up');Start-Sleep -Seconds 5
    Send-XR @('btn b press 180');Start-Sleep -Seconds 3
    Finish-Take
} finally {Stop-Take}
