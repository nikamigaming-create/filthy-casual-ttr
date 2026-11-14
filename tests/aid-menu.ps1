. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
# Fixture-only food; no user save or persisted inventory changes.
Send-Game @('player.AddItem 0001519B 2') | Out-Null
function Select-Aid([int]$Form) {
    Park-Hands
    Send-XR @('click l press 180')
    Start-Sleep -Milliseconds 300
    for($page=0;$page -lt 4;$page++) {
        $a=Get-Addon
        if($a.previews.Count -gt 1){
            $dx=$a.previews[1].center[0]-$a.previews[0].center[0]
            $dy=$a.previews[1].center[1]-$a.previews[0].center[1]
            $length=[Math]::Sqrt($dx*$dx+$dy*$dy)
            if($length -gt 0.001){$script:aidRight=@(($dx/$length),($dy/$length))}
        }
        $preview=@($a.previews | Where-Object form -eq $Form)
        if($preview.Count){Touch-Slot $preview[0].slot 0 $script:aidRight | Out-Null; Start-Sleep -Milliseconds 650; return}
        Send-XR @('btn a press 180'); Start-Sleep -Milliseconds 250
    }
    throw "Aid form $Form not found"
}
$count=Get-Native 'player.GetItemCount 0001519B'
Select-Aid 86427
$held=Get-Addon
Assert-Equal $held.nativeGrabHand 1 'Food is in native right hand'
Assert-Equal $held.aidGripLatch $true 'Food is held without physical grip'
Assert-Equal (Get-Native 'player.GetItemCount 0001519B') ($count-1) 'One food instance transferred to hand'
Send-Game @('TapKey 1') | Out-Null
Start-Sleep -Milliseconds 700
Assert-Equal (Get-Native 'MenuMode') 1 'Native pause menu opened'
Send-Game @('CloseAllMenus') | Out-Null
Start-Sleep -Milliseconds 1000
$after=Get-Addon
Write-Output "Menu return: beforeRef=$($held.nativeGrabRef) afterRef=$($after.nativeGrabRef) latch=$($after.aidGripLatch)"
Assert-Equal $after.nativeGrabRef $held.nativeGrabRef 'Same native food remains held after menu'
Assert-Equal $after.aidGripLatch $true 'Food grip retained after menu'
# Clear through the native cap path, then verify native Stimpak activation.
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 300
Touch-Slot 6 1 | Out-Null
Start-Sleep -Milliseconds 400
Assert-Equal (Get-Native 'player.GetItemCount 0001519B') $count 'Cap returns food without loss or duplication'
Select-Aid 86427
Send-Game @('player.damageav health 50') | Out-Null
$health=Get-Native 'player.GetAV Health'
Send-XR @('btn y down')
Start-Sleep -Milliseconds 1200
Assert-Equal (Get-Addon).nativeGrabRef 0 'Y clears the consumed food from the hand'
Assert-Equal (Get-Native 'MenuMode') 0 'Y consume does not open Pause'
Assert-Equal (Get-Native 'player.GetItemCount 0001519B') ($count-1) 'Held Y consumes exactly one selected food'
Send-XR @('btn y up')
Start-Sleep -Seconds 2
Assert-Equal ((Get-Native 'player.GetAV Health') -gt $health) $true 'Food applies its normal native healing effect'
Select-Aid 86427
Send-XR @('grip r 1')
Start-Sleep -Milliseconds 250
Send-XR @('grip r 0')
Start-Sleep -Milliseconds 500
Assert-Equal (Get-Addon).nativeGrabRef 0 'Deliberate grip release still drops food'
Send-XR @('btn y press 180')
Start-Sleep -Milliseconds 400
Assert-Equal (Get-Native 'MenuMode') 1 'Y remains Pause after food is released'
Send-Game @('CloseAllMenus') | Out-Null
Start-Sleep -Milliseconds 500
Select-Aid 86377
Assert-Equal (Get-Addon).nativeStimHand 1 'Stimpak uses native right-hand live-Stim'
$doses=Get-Native 'player.GetItemCount Stimpak'
Send-Game @('player.damageav health 50') | Out-Null
Send-XR @('head height 1.6')
Start-Sleep -Milliseconds 350
Send-XR @('hand r grip pose 0.10 1.25 -0.50 0 0 0','trigger r 1')
Start-Sleep -Milliseconds 250
Send-XR @('hand r to grip 0.02 1.22 0.03 0 0 0 100')
Start-Sleep -Milliseconds 500
Send-XR @('trigger r 0')
Assert-Equal (Get-Native 'player.GetItemCount Stimpak') ($doses-1) 'Right trigger and body stab consume one native Stimpak'
Assert-Equal (Get-Addon).nativeStimHand -1 'Native use clears Stimpak hand'
Send-XR @('stick l 0 1')
Start-Sleep -Milliseconds 650
Assert-Equal (Get-Native 'player.IsRunning') 1 'Full left stick runs without clicking'
Send-XR @('stick l center')
