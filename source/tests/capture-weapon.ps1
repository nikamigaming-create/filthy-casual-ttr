. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.6','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('hand l grip pose -0.25 1.16 -0.28 -8 50 -8','hand r grip pose 0.20 1.22 -0.30 5 55 5','video on')
$s=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
$record=Start-Process -FilePath python.exe -ArgumentList @("`"$PSScriptRoot\record-eyes.py`"",'--pid',$s.pid,'--out',"`"$script:Repo\outputs\guide\weapon-stereo.mp4`"",'--seconds',17,'--fps',30) -WindowStyle Hidden -PassThru
$clock=[Diagnostics.Stopwatch]::StartNew();$events=[Collections.Generic.List[object]]::new()
try {
    Start-Sleep -Seconds 1
    $events.Add(@{seconds=$clock.Elapsed.TotalSeconds;action='Open weapons'})
    Send-XR @('click r press 180');Start-Sleep -Milliseconds 1500
    $events.Add(@{seconds=$clock.Elapsed.TotalSeconds;action='Touch pistol'})
    $a=Get-Addon;$target=@($a.previews|Where-Object slot -eq 1)[0].center
    $dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1];$len=[Math]::Sqrt($dx*$dx+$dy*$dy);$rx=$dx/$len;$ry=$dy/$len
    foreach($near in .18,0){
        $a=Get-Addon;if($a.openHand -lt 0){break}
        $s=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
        $ex=$target[0]-$a.palms[1][0];$ey=$target[1]-$a.palms[1][1];$ez=$target[2]-$a.palms[1][2]
        $px=$s.handR.pos[0]+($rx*$ex+$ry*$ey)/70;$py=$s.handR.pos[1]+$ez/70;$pz=$s.handR.pos[2]-(-$ry*$ex+$rx*$ey)/70+$near
        Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 5 55 5 1000" -f $px,$py,$pz));Start-Sleep -Milliseconds 1150
    }
    Start-Sleep -Milliseconds 500
    Assert-Equal (Get-Addon).weaponHand 1 'Natural-pose take uses native right-hand weapon'
    Send-XR @('hand r to grip 0.12 1.39 -0.43 0 55 0 1200');Start-Sleep -Milliseconds 1500
    $events.Add(@{seconds=$clock.Elapsed.TotalSeconds;action='Native firing'})
    Send-Game @('player.SetCurrentAmmoRounds 1') | Out-Null
    Send-XR @('trigger r pull 180');Start-Sleep -Milliseconds 2200
    Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 12 'Natural-pose take auto reloads'
    Send-XR @('trigger r pull 180');Start-Sleep -Milliseconds 700
    Send-XR @('hand r to grip 0.17 1.37 -0.41 7 50 4 1800');Start-Sleep -Milliseconds 2000
    $events.Add(@{seconds=$clock.Elapsed.TotalSeconds;action='Finished'})
    $events | ConvertTo-Json | Set-Content -LiteralPath "$script:Repo\outputs\guide\weapon-timeline.json"
    if(!$record.WaitForExit(10000)){throw 'Bounded weapon recorder timed out'}
} finally {Send-XR @('video off');if(!$record.HasExited){Stop-Process -Id $record.Id -Force}}
