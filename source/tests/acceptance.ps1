# Run only against a live launch-simulator.cmd session with the private QA fixture.
# Never use a real save profile. This script changes inventory/ammunition in QA.
. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Assert-Equal (Get-Addon).heightOffset 0 'Neutral height persists after restart'
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 350
Assert-Equal (Get-Addon).openHand 0 'Tap opens a latched aid wheel'
Send-XR @('head pos 1.8 1.1 0')
Start-Sleep -Milliseconds 350
Assert-Equal (Get-Addon).openHand -1 'Physical walk-away dismisses wheel'
Send-XR @('head pos 0 1.1 0')
Start-Sleep -Milliseconds 250
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
Touch-Slot 1 0 | Out-Null
Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).weaponHand 1 'Left touch equips weapon in right hand'
Assert-Equal (Get-Native 'player.GetEquippedObject 5') 17231 'Touch selects pistol'
Assert-Equal (Get-Native 'player.IsWeaponOut') 1 'Touch draws pistol'
Send-Game @('player.SetCurrentAmmoRounds 1') | Out-Null
Send-XR @('trigger r pull 220')
Start-Sleep -Seconds 2
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'Empty pistol reloads automatically'
Send-XR @('trigger r pull 220')
Start-Sleep -Milliseconds 500
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 11 'Pistol fires after automatic reload'
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 300
Touch-Slot 1 1 | Out-Null
Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Retouch drawn weapon holsters it'
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 300
$count=Get-Native 'player.GetItemCount Buffout'
Send-XR @('trigger r 1')
Touch-Slot 0 0 | Out-Null
Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).aidHand 1 'Touch puts aid in right hand'
Assert-Equal (Get-Addon).nativeGrabHand 1 'Aid uses original TTWVR right-hand grab'
Assert-Equal (Get-Addon).nativeGrabScale 1 'Native aid retains authored size'
Assert-Equal (Get-Native 'player.GetItemCount Buffout') ($count-1) 'Exactly one real item transfers from inventory to native hand'
Save-Eyes 'native-release-held-aid'
Send-XR @('trigger r 0')
Start-Sleep -Milliseconds 180
Send-XR @('trigger r pull 220')
Start-Sleep -Milliseconds 300
Assert-Equal (Get-Native 'player.GetItemCount Buffout') ($count-1) 'Selector does not add a custom consume action'
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 300
Touch-Slot 1 0 | Out-Null
Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).nativeGrabRef 0 'Weapon selection clears previous native aid'
Assert-Equal (Get-Native 'player.GetItemCount Buffout') $count 'Replacement returns held aid to inventory'
Assert-Equal (Get-Native 'player.IsWeaponOut') 1 'Weapon draws after native aid replacement'
Park-Hands
Send-XR @('click l down','click r down')
Start-Sleep -Milliseconds 750
Send-XR @('click l up','click r up','btn a press 180')
Start-Sleep -Milliseconds 300
Assert-Equal (Get-Addon).heightOffset 0.55 'Seated calibration computes 55 cm'
Save-Eyes 'native-release-height-calibrated'
Send-XR @('btn x press 180')
Start-Sleep -Milliseconds 250
Send-XR @('btn b press 180')
Start-Sleep -Milliseconds 250
Assert-Equal (Get-Addon).heightOffset 0 'Simulator offset cleared before play'
Assert-Equal (Get-Native 'MenuMode') 0 'Height buttons do not leak to menus'
Send-XR @('head height 1.6')
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 300
$a=Get-Addon
$dx=$a.previews[1].center[0]-$a.previews[0].center[0]
$dy=$a.previews[1].center[1]-$a.previews[0].center[1]
$length=[Math]::Sqrt($dx*$dx+$dy*$dy)
$right=@(($dx/$length),($dy/$length))
Send-XR @('btn a press 180')
Start-Sleep -Milliseconds 300
Touch-Slot 0 0 $right | Out-Null
Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).nativeStimHand 1 'Stimpak selection uses native right-hand live-Stim'
Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Native Stimpak replaces drawn weapon'
Assert-Equal (Get-Native 'player.GetItemCount Stimpak') 5 'Selecting native Stimpak does not consume'
Send-Game @('player.damageav health 50') | Out-Null
Send-XR @('hand r grip pose 0.10 1.25 -0.50 0 0 0','trigger r 1')
Start-Sleep -Milliseconds 250
Send-XR @('hand r to grip 0.02 1.22 0.03 0 0 0 100')
Start-Sleep -Milliseconds 500
Send-XR @('trigger r 0')
Assert-Equal (Get-Native 'player.GetItemCount Stimpak') 4 'Native trigger and body stab consumes one dose'
Assert-Equal (Get-Addon).nativeStimHand -1 'Native use clears Stimpak from hand'
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 300
Touch-Slot 0 0 | Out-Null
Start-Sleep -Milliseconds 600
Send-XR @('grip r 1','hand r grip pose 0.20 1.45 0.15 0 0 0')
Start-Sleep -Milliseconds 500
Send-XR @('grip r 0')
Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).nativeGrabRef 0 'Ordinary native backpack release clears aid'
Assert-Equal (Get-Native 'player.GetItemCount Buffout') $count 'Native backpack restores original inventory count'
