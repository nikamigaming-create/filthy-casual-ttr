# Focused 1.2 checks. The runner loads only the isolated fixture and closes it.
. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
$a=Get-Addon
Assert-Equal $a.selectorPopups $false 'Selector hints default off'
Assert-Equal $a.popupVisible $false 'Quiet selector has no addon popup'
$cap=@($a.previews | Where-Object slot -eq 6)
Assert-Equal $cap.Count 1 'Weapon page has one center cap'
Assert-Equal ([Math]::Round((($cap[0].extent | Measure-Object -Maximum).Maximum)/0.7,1)) 12 'Center cap diameter is 12 cm'
Save-Eyes 'cap-weapons-quiet'
Touch-Slot 1 0 | Out-Null
Start-Sleep -Milliseconds 600
Assert-Equal (Get-Native 'player.IsWeaponOut') 1 'Outer weapon selection still draws normally'
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
Touch-Slot 6 1 | Out-Null
Assert-Equal (Get-Addon).openHand -1 'Touching center closes selector'
Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Center cap holsters drawn weapon'
Assert-Equal (Get-Addon).popupVisible $false 'Quiet deselection has no popup'
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 350
$count=Get-Native 'player.GetItemCount Buffout'
Save-Eyes 'cap-aid-quiet'
Touch-Slot 0 0 | Out-Null
Start-Sleep -Milliseconds 650
Assert-Equal (Get-Addon).nativeGrabHand 1 'Outer aid still uses native right hand'
Assert-Equal (Get-Native 'player.GetItemCount Buffout') ($count-1) 'One aid instance transferred to hand'
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 350
Touch-Slot 6 0 | Out-Null
Assert-Equal (Get-Addon).nativeGrabRef 0 'Left-hand cap touch clears native grabbed aid'
Assert-Equal (Get-Native 'player.GetItemCount Buffout') $count 'Center cap returns held aid without consumption'
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 350
$a=Get-Addon
$dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1]
$length=[Math]::Sqrt($dx*$dx+$dy*$dy);$right=@(($dx/$length),($dy/$length))
Send-XR @('btn a press 180')
Start-Sleep -Milliseconds 350
Assert-Equal (@((Get-Addon).previews | Where-Object slot -eq 6).Count) 1 'Center cap persists on second page'
Touch-Slot 0 0 $right | Out-Null
Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).nativeStimHand 1 'Stimpak is ready in native right hand'
$stims=Get-Native 'player.GetItemCount Stimpak'
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
Touch-Slot 6 1 | Out-Null
Assert-Equal (Get-Addon).nativeStimHand -1 'Weapon-wheel cap also clears held Stimpak'
Assert-Equal (Get-Native 'player.GetItemCount Stimpak') $stims 'Center cap never consumes Stimpak'
Park-Hands
Send-XR @('click l down','click r down')
Start-Sleep -Milliseconds 750
Send-XR @('click l up','click r up')
Start-Sleep -Milliseconds 200
Assert-Equal (Get-Addon).popupVisible $true 'Height panel remains visible with selector hints off'
Send-XR @('btn a press 180')
Start-Sleep -Milliseconds 250
Assert-Equal (Get-Addon).popupVisible $true 'Height calibration popup remains visible'
Send-XR @('btn x press 180')
Start-Sleep -Milliseconds 250
Assert-Equal (Get-Addon).popupVisible $true 'Height reset popup remains visible'
Send-XR @('btn y press 180')
Start-Sleep -Milliseconds 250
Assert-Equal (Get-Addon).selectorPopups $true 'Y enables selector hints from height panel'
Save-Eyes 'cap-height-popup-settings'
Send-XR @('btn b press 180')
Start-Sleep -Milliseconds 200
Assert-Equal (Get-Addon).popupVisible $true 'Height save confirmation remains visible'
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
Assert-Equal (Get-Addon).popupVisible $true 'Enabled selector hints display again'
Send-XR @('btn b press 180','click l down','click r down')
Start-Sleep -Milliseconds 750
Send-XR @('click l up','click r up','btn y press 180')
Start-Sleep -Milliseconds 250
Assert-Equal (Get-Addon).selectorPopups $false 'Y disables selector hints again'
Send-XR @('btn b press 180')
Start-Sleep -Milliseconds 200
Assert-Equal (Get-Addon).popupVisible $true 'Height save message stays on after hints disabled'
Assert-Equal (Get-Native 'MenuMode') 0 'Popup toggle does not leak to game menus'
# Empty inventory is deliberately confined to this unsaved QA fixture.
Send-Game @('player.RemoveAllItems') | Out-Null
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 350
Assert-Equal (Get-Addon).previews.Count 1 'Empty inventory still has center cap'
Send-XR @('btn a press 180')
Start-Sleep -Milliseconds 250
Assert-Equal (Get-Addon).previews.Count 1 'Paging an empty selector is safe'
Touch-Slot 6 1 $right | Out-Null
Assert-Equal (Get-Addon).openHand -1 'Empty selector cap closes cleanly'
