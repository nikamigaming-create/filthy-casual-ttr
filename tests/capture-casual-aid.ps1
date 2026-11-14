. "$PSScriptRoot\casual-guide-helpers.ps1"
Load-Guide
Send-Game @('player.AddItem 0001519B 2','player.damageav health 50') | Out-Null
Start-Take 'food-stimpak' 110
try {
    Mark 'Open food';Choose-Aid 86427
    Assert-Equal (Get-Addon).nativeGrabHand 1 'Food uses native right hand'
    Aim-Item;Start-Sleep -Seconds 5
    Save-Eyes 'casual-food'
    $held=Get-Addon;$before=Get-Native 'player.GetItemCount 0001519B'
    Mark 'Menu retention'
    Send-Game @('prid 00104C79','Activate player') | Out-Null
    Start-Sleep -Milliseconds 700
    $dialogue=Get-Native 'MenuMode 1009'
    Write-Output "Native conversation menu active: $dialogue"
    if(!(Get-Native 'MenuMode')){Send-Game @('TapKey 1') | Out-Null}
    Start-Sleep -Seconds 3
    Send-Game @('CloseAllMenus') | Out-Null;Start-Sleep -Seconds 4
    Assert-Equal (Get-Addon).nativeGrabRef $held.nativeGrabRef 'Same food remains held after menu'
    Mark 'Y consume';Send-XR @('btn y press 180');Start-Sleep -Seconds 5
    Assert-Equal (Get-Addon).nativeGrabRef 0 'Y consumes held food'
    Assert-Equal (Get-Native 'player.GetItemCount 0001519B') $before 'One food used without consuming a second inventory item'
    Assert-Equal (Get-Native 'MenuMode') 0 'Y does not leak to Pause'
    Mark 'Open Stimpak';Choose-Aid 86377
    Assert-Equal (Get-Addon).nativeStimHand 1 'Live Stimpak in right hand'
    Aim-Item;Start-Sleep -Seconds 5
    $doses=Get-Native 'player.GetItemCount Stimpak'
    Mark 'Trigger then jab';Send-XR @('head to 0 1.1 0 0 -35 0 1600');Start-Sleep -Milliseconds 1800
    Send-XR @('hand r to grip 0.10 0.70 -0.50 0 45 0 1500');Start-Sleep -Seconds 2
    Send-XR @('trigger r 1');Start-Sleep -Milliseconds 500
    Send-XR @('hand r to grip 0.02 0.67 0.03 0 30 0 600');Start-Sleep -Seconds 2
    Send-XR @('trigger r 0');Start-Sleep -Seconds 2
    Assert-Equal (Get-Native 'player.GetItemCount Stimpak') ($doses-1) 'Native body stab uses one dose'
    Rest-Hands;Send-XR @('head to 0 1.1 0 0 -3 0 1700');Start-Sleep -Seconds 3
    Mark 'Quiet height panel';Send-XR @('click l down','click r down');Start-Sleep -Milliseconds 750
    Send-XR @('click l up','click r up');Start-Sleep -Seconds 5
    Send-XR @('btn b press 180');Start-Sleep -Seconds 3
    Finish-Take
} finally {Stop-Take}
