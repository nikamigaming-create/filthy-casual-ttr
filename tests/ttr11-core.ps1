. "$PSScriptRoot\runtime-helpers.ps1"
. "$PSScriptRoot\ttr11-touch-helper.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 0 0','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 5
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
$originalHeight=(Get-Addon).heightOffset
Send-Game @('sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
$a=Get-Addon;$form=$a.previews[0].form
Touch-Slot 0 1|Out-Null;Start-Sleep -Milliseconds 800
Assert-Equal (Get-Native 'player.GetEquippedObject 5') $form 'Real touch equips intended weapon'
Assert-Equal (Get-Addon).weaponHand 1 'Touch-selected weapon uses native right hand'
Park-Hands
Send-Game @('sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Touch-Slot 6 1|Out-Null;Start-Sleep -Milliseconds 400
Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Center cap clears weapon'
Park-Hands
Send-Game @('sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 0000434F')|Out-Null;Start-Sleep -Milliseconds 800
if((Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 300}
Send-Game @('player.SetCurrentAmmoRounds 1')|Out-Null
Send-XR @('trigger r pull 220');Start-Sleep -Seconds 2
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'Native automatic reload fills pistol'
Send-XR @('trigger r pull 220');Start-Sleep -Milliseconds 400
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 11 'Native pistol fires after reload'
Park-Hands
$count=Get-Native 'player.GetItemCount Buffout'
Send-Game @('sc_aid')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 00015163')|Out-Null;Start-Sleep -Milliseconds 800
Assert-Equal (Get-Addon).nativeGrabHand 1 'Aid transfers to native right hand'
Assert-Equal (Get-Addon).nativeGrabScale 1 'Aid retains normal authored scale'
Assert-Equal (Get-Native 'player.GetItemCount Buffout') ($count-1) 'One actual aid instance transferred'
Save-Eyes 'ttr11-native-aid'
Park-Hands
Send-Game @('sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Touch-Slot 6 1|Out-Null;Start-Sleep -Milliseconds 400
Assert-Equal (Get-Addon).nativeGrabRef 0 'Center cap clears held aid'
Assert-Equal (Get-Native 'player.GetItemCount Buffout') $count 'Aid returned without consuming or duplicating'
Park-Hands
$doses=Get-Native 'player.GetItemCount Stimpak'
Send-Game @('sc_aid')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 00015169')|Out-Null;Start-Sleep -Milliseconds 800
Assert-Equal (Get-Addon).nativeStimHand 1 'Stimpak enters native right-hand path'
Assert-Equal (Get-Native 'player.GetItemCount Stimpak') $doses 'Selecting Stimpak does not consume it'
Park-Hands
Send-Game @('sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Touch-Slot 6 1|Out-Null;Start-Sleep -Milliseconds 400
Assert-Equal (Get-Addon).nativeStimHand -1 'Center cap cancels native Stimpak normally'
Assert-Equal (Get-Native 'player.GetItemCount Stimpak') $doses 'Cancelling Stimpak preserves inventory'
Send-XR @('btn menu down');Start-Sleep -Milliseconds 1250;Send-XR @('btn menu up');Start-Sleep -Milliseconds 300
Assert-Equal (Get-Addon).heightMenu $true 'Native Menu hold opens addon settings'
Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300
Assert-Equal (Get-Addon).heightOffset 0.55 'Seated calibration computes expected offset'
Send-XR @('btn b press 180');Start-Sleep -Milliseconds 300
Assert-Equal (Get-Addon).heightMenu $false 'Settings close normally'
Assert-Equal (Get-Native 'MenuMode') 0 'Settings controls do not leak into native menus'
Save-Eyes 'ttr11-height'
Write-Output 'PASS TTR1.1 core native handoff, reload and height'
