. "$PSScriptRoot\casual-guide-helpers.ps1"
Load-Guide
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 400
Smooth-Touch 1
Rest-Hands
Aim-Item
Send-Game @('player.SetCurrentAmmoRounds 5') | Out-Null
Start-Take 'native-options' 110
try {
    Mark 'Short reload';Start-Sleep -Seconds 4
    Send-XR @('btn a press 180');Start-Sleep -Seconds 3
    Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'Recorded short A native reload'
    Mark 'Manual reload mode';Start-Sleep -Seconds 3
    Send-XR @('btn a down');Start-Sleep -Milliseconds 1400;Send-XR @('btn a up');Start-Sleep -Seconds 3
    Assert-Equal (Get-Addon).manualReload $true 'Recorded live manual reload mode'
    Mark 'Seated ammo';Send-XR @('hand l to grip -0.4 0.55 -0.08 0 30 0 1200','hand r to grip 0.4 0.70 -0.22 0 55 0 1200','head to 0 1.1 0 0 -55 0 1600');Start-Sleep -Seconds 6
    $a=Get-Addon
    Assert-Equal $a.pouch.shown $true 'Native spare magazine is available below the seated player'
    Assert-Equal $a.pouch.primary.culled $false 'Native spare magazine is unculled'
    Save-Eyes 'casual-seated-ammo'
    Send-XR @('head to 0 1.1 0 15 -55 0 1400');Start-Sleep -Seconds 3
    Send-XR @('head to 0 1.1 0 0 -55 0 1400');Start-Sleep -Seconds 2
    Mark 'Grab ammo'
    # Reach the native geometry center with the left palm, using the measured
    # player/world basis from the selector. Never write native magazine state.
    for($attempt=0;$attempt -lt 3;$attempt++){
        $a=Get-Addon;$target=$a.pouch.center
        if($target.Count -ne 3){throw 'No native spare target'}
        $s=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
        $ex=$target[0]-$a.palms[0][0];$ey=$target[1]-$a.palms[0][1];$ez=$target[2]-$a.palms[0][2]
        $px=$s.handL.pos[0]+($script:guideBasis[0]*$ex+$script:guideBasis[1]*$ey)/70
        $py=$s.handL.pos[1]+$ez/70
        $pz=$s.handL.pos[2]-(-$script:guideBasis[1]*$ex+$script:guideBasis[0]*$ey)/70
        Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} 0 30 0 1500" -f $px,$py,$pz));Start-Sleep -Milliseconds 1700
    }
    Send-XR @('grip l 1');Start-Sleep -Milliseconds 500
    Send-XR @('hand l to grip -0.15 0.9 -0.40 0 30 0 1600');Start-Sleep -Seconds 3
    $held=Get-Addon
    $held | ConvertTo-Json -Depth 8 | Set-Content "$script:Repo\evidence\native-spare-held.json"
    Write-Output "Native spare interaction mode after pickup: $($held.pouch.mode)"
    Save-Eyes 'casual-ammo-held'
    Mark 'Automatic reload mode'
    Send-XR @('btn a down');Start-Sleep -Milliseconds 1400;Send-XR @('btn a up');Start-Sleep -Seconds 3
    Assert-Equal (Get-Addon).manualReload $false 'Recorded toggle returns to automatic even with native ammo held'
    Send-XR @('grip l 0');Rest-Hands
    Send-XR @('head to 0 1.1 0 0 -3 0 1500');Aim-Item;Start-Sleep -Seconds 3
    Mark 'Native grip takeover';Start-Sleep -Seconds 3
    Send-XR @('grip r 1');Start-Sleep -Seconds 2
    Assert-Equal (Get-Addon).weaponHand -1 'Recorded squeeze returns gun grip to native control'
    Send-XR @('grip r 0');Start-Sleep -Seconds 4
    Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Recorded native release holsters the gun'
    Finish-Take
} finally {Stop-Take}
