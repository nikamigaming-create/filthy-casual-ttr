# One bounded, native-input demonstration. No save or inventory-recovery logic.
. "$PSScriptRoot\runtime-helpers.ps1"
$events=[Collections.Generic.List[object]]::new()
$clock=[Diagnostics.Stopwatch]::new()
function Mark([string]$Name){$events.Add([ordered]@{seconds=$clock.Elapsed.TotalSeconds;action=$Name})}
function Rest-Hands {
    Send-XR @('hand l to grip -0.25 1.16 -0.28 -8 50 -8 700','hand r to grip 0.20 1.22 -0.30 5 55 5 700')
    Start-Sleep -Milliseconds 800
}
function Smooth-Touch([int]$Slot,[double[]]$Basis=@()) {
    $a=Get-Addon
    if($Basis.Count -ne 2){
        $dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1]
        $length=[Math]::Sqrt($dx*$dx+$dy*$dy);$Basis=@(($dx/$length),($dy/$length))
    }
    $target=@($a.previews | Where-Object slot -eq $Slot)[0].center
    # Approach from in front of the selector plane, then touch forward. This
    # avoids sweeping through another item's collision volume on the way in.
    foreach($near in 0.18,0.0){
        $a=Get-Addon;if($a.openHand -lt 0){break}
        $s=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
        $ex=$target[0]-$a.palms[1][0];$ey=$target[1]-$a.palms[1][1];$ez=$target[2]-$a.palms[1][2]
        $px=$s.handR.pos[0]+($Basis[0]*$ex+$Basis[1]*$ey)/70
        $py=$s.handR.pos[1]+$ez/70
        $pz=$s.handR.pos[2]-(-$Basis[1]*$ex+$Basis[0]*$ey)/70+$near
        Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 5 55 5 1000" -f $px,$py,$pz))
        Start-Sleep -Milliseconds 1150
    }
    Start-Sleep -Milliseconds 500
    Assert-Equal (Get-Addon).openHand -1 'Touch closes selector without grip'
}
Send-XR @('capture off','pace free 60','head height 1.6','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Rest-Hands
Send-XR @('video on')
$s=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
$record=Start-Process -FilePath python.exe -ArgumentList @("`"$PSScriptRoot\record-eyes.py`"",'--pid',$s.pid,'--out',"`"$script:Repo\outputs\guide\gameplay-stereo.mp4`"",'--seconds',70,'--fps',30) -WindowStyle Hidden -PassThru
$clock.Start()
try {
    Start-Sleep -Seconds 2
    Mark 'Open weapons';Send-XR @('click r press 180');Start-Sleep -Milliseconds 1100
    Send-XR @('head to 0.015 1.60 0.01 4 -2 0 1300');Start-Sleep -Milliseconds 1400
    Send-XR @('head to 0 1.60 0 0 0 0 1000');Start-Sleep -Milliseconds 1100
    Mark 'Touch pistol';Smooth-Touch 1
    Assert-Equal (Get-Addon).weaponHand 1 'Guide selects weapon in right hand'
    Assert-Equal (Get-Native 'player.GetEquippedObject 5') 17231 'Guide selects the intended pistol'
    Send-XR @('hand r to grip 0.13 1.39 -0.43 -5 8 0 900');Start-Sleep -Milliseconds 1000
    Mark 'Native firing';Send-Game @('player.SetCurrentAmmoRounds 1') | Out-Null
    Send-XR @('trigger r pull 180');Start-Sleep -Seconds 2
    Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'Guide demonstrates automatic reload'
    Send-XR @('trigger r pull 180');Start-Sleep -Milliseconds 600
    Mark 'Open consumables';Rest-Hands;Send-XR @('click l press 180');Start-Sleep -Milliseconds 1200
    Mark 'Touch Buffout';Smooth-Touch 0
    Assert-Equal (Get-Addon).nativeGrabHand 1 'Guide aid uses native right-hand grab'
    Assert-Equal (Get-Addon).nativeGrabScale 1 'Guide held aid has native scale'
    Send-XR @('hand r to grip 0.10 1.39 -0.43 0 55 0 1000');Start-Sleep -Milliseconds 1100
    Mark 'Native held item';Send-XR @('hand r to grip 0.16 1.42 -0.47 9 48 6 1300','head to 0.01 1.6 0 4 -2 0 1400');Start-Sleep -Milliseconds 1500
    Send-XR @('hand r to grip 0.10 1.39 -0.43 0 55 0 1300','head to 0 1.6 0 0 0 0 1400');Start-Sleep -Milliseconds 1500
    Mark 'Native backpack stow';Send-XR @('grip r 1','hand r to grip 0.20 1.45 0.15 0 55 0 900');Start-Sleep -Milliseconds 1000
    Send-XR @('grip r 0');Start-Sleep -Milliseconds 700
    Assert-Equal (Get-Addon).nativeGrabRef 0 'Guide native backpack stows aid'
    Rest-Hands;Mark 'Page to Stimpak';Send-XR @('click l press 180');Start-Sleep -Milliseconds 650
    $a=Get-Addon;$dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1];$len=[Math]::Sqrt($dx*$dx+$dy*$dy);$basis=@(($dx/$len),($dy/$len))
    Send-XR @('btn a press 180');Start-Sleep -Milliseconds 900
    Mark 'Touch Stimpak';Smooth-Touch 0 $basis
    Assert-Equal (Get-Addon).nativeStimHand 1 'Guide Stimpak uses native right hand'
    Send-XR @('hand r to grip 0.10 1.35 -0.43 0 45 0 900');Start-Sleep -Milliseconds 2000
    Mark 'Put Stimpak away';Send-XR @('grip r 1');Start-Sleep -Milliseconds 200;Send-XR @('grip r 0');Start-Sleep -Milliseconds 500
    Rest-Hands;Mark 'Seated height';Send-XR @('click l down','click r down');Start-Sleep -Milliseconds 750
    Send-XR @('click l up','click r up');Start-Sleep -Milliseconds 1800
    Send-XR @('btn a press 180');Start-Sleep -Milliseconds 1600
    Assert-Equal (Get-Addon).heightOffset 0.05 'Guide calibrates to target height'
    Send-XR @('btn x press 180');Start-Sleep -Milliseconds 400;Send-XR @('btn b press 180');Start-Sleep -Milliseconds 800
    Assert-Equal (Get-Addon).heightOffset 0 'Guide leaves play height neutral'
    Mark 'Finished';
    $events | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$script:Repo\outputs\guide\timeline.json"
    if(!$record.WaitForExit(35000)){throw 'Recorder did not finish its bounded take'}
    Assert-Equal $record.ExitCode 0 'Guide recording completed'
} finally {
    Send-XR @('video off')
    if(!$record.HasExited){Stop-Process -Id $record.Id -Force}
}
