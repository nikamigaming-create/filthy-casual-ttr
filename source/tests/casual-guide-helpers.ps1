. "$PSScriptRoot\runtime-helpers.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\casual-guide'
New-Item -ItemType Directory -Force $script:Guide | Out-Null
$script:events=[Collections.Generic.List[object]]::new()
$script:takeClock=[Diagnostics.Stopwatch]::new()
$script:eyeHeight=1.1
function Mark([string]$Name){$script:events.Add([ordered]@{seconds=$script:takeClock.Elapsed.TotalSeconds;action=$Name});$script:events | ConvertTo-Json | Set-Content "$script:Guide\$script:takeName-timeline.json";Write-Output "CAPTURE: $Name"}
function Rest-Hands {
    $y=$script:eyeHeight-.40
    Send-XR @("hand l to grip -0.25 $y -0.28 -8 50 -8 1400","hand r to grip 0.20 $y -0.30 5 55 5 1400")
    Start-Sleep -Milliseconds 1500
}
function Aim-Item {
    $y=$script:eyeHeight-.19
    Send-XR @("hand r to grip 0.12 $y -0.43 0 55 0 1800")
    Start-Sleep -Milliseconds 2000
}
function Smooth-Touch([int]$Slot){
    $a=Get-Addon
    if($a.previews.Count -gt 1){
        $dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1]
        $len=[Math]::Sqrt($dx*$dx+$dy*$dy)
        if($len -gt .001){$script:guideBasis=@(($dx/$len),($dy/$len))}
    }
    if($script:guideBasis.Count -ne 2){throw 'No valid guide selector basis'}
    $target=@($a.previews | Where-Object slot -eq $Slot)[0].center
    $s=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
    Send-XR @(("hand r to grip {0:F5} {1:F5} 0.10 5 55 5 900" -f $s.handR.pos[0],$s.handR.pos[1]))
    Start-Sleep -Milliseconds 1000
    foreach($near in .35,0){
        $a=Get-Addon;if($a.openHand -lt 0){break}
        $s=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
        $ex=$target[0]-$a.palms[1][0];$ey=$target[1]-$a.palms[1][1];$ez=$target[2]-$a.palms[1][2]
        $px=$s.handR.pos[0]+($script:guideBasis[0]*$ex+$script:guideBasis[1]*$ey)/70
        $py=$s.handR.pos[1]+$ez/70
        $pz=$s.handR.pos[2]-(-$script:guideBasis[1]*$ex+$script:guideBasis[0]*$ey)/70+$near
        Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 5 55 5 1600" -f $px,$py,$pz))
        Start-Sleep -Milliseconds 1750
    }
    Start-Sleep -Milliseconds 700
    Assert-Equal (Get-Addon).openHand -1 'Slow touch closes the selector'
}
function Choose-Aid([int]$Form){
    Rest-Hands; Send-XR @('click l press 180');Start-Sleep -Seconds 4
    for($page=0;$page -lt 4;$page++){
        $a=Get-Addon
        $dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1]
        $len=[Math]::Sqrt($dx*$dx+$dy*$dy);if($len -gt .001){$script:guideBasis=@(($dx/$len),($dy/$len))}
        $p=@($a.previews | Where-Object form -eq $Form)
        if($p.Count){Smooth-Touch $p[0].slot;return}
        Send-XR @('btn a press 180');Start-Sleep -Seconds 3
    }
    throw 'Guide aid not found'
}
function Start-Take([string]$Name,[int]$Seconds){
    Send-XR @('video on')
    $s=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
    $script:takeName=$Name
    $script:takeLimit=$Seconds
    $provenance=[ordered]@{
        pid=$s.pid;startedUtc=[DateTime]::UtcNow.ToString('o')
        nativeSHA256=(Get-FileHash -LiteralPath $(if($script:CaptureNativePath){$script:CaptureNativePath}else{'D:\TTWVR\MO2\mods\TTWVR\NVSE\Plugins\TTWVR.dll'}) -Algorithm SHA256).Hash
        addonPath=$(if($env:FC_CAPTURE_ADDON_PATH){$env:FC_CAPTURE_ADDON_PATH}elseif($script:CaptureAddonPath){$script:CaptureAddonPath}else{'D:\TTWVR\MO2\mods\TTWVR - Tutorial Development\NVSE\Plugins\SeatedControls.dll'})
        addonSHA256=(Get-FileHash -LiteralPath $(if($env:FC_CAPTURE_ADDON_PATH){$env:FC_CAPTURE_ADDON_PATH}elseif($script:CaptureAddonPath){$script:CaptureAddonPath}else{'D:\TTWVR\MO2\mods\TTWVR - Tutorial Development\NVSE\Plugins\SeatedControls.dll'}) -Algorithm SHA256).Hash
        capture='final composed eyes';nativeActions=$true
    }
    $provenance|ConvertTo-Json|Set-Content "$script:Guide\$Name-provenance.json"

    $script:stopFile="$script:Guide\$Name-stop.txt"
    if(Test-Path -LiteralPath $script:stopFile){Remove-Item -LiteralPath $script:stopFile}
    $script:record=Start-Process python.exe -ArgumentList @("`"$PSScriptRoot\record-eyes.py`"",'--pid',$s.pid,'--out',"`"$script:Guide\$Name-stereo.mp4`"",'--seconds',$Seconds,'--fps',30,'--audio','--stop-file',"`"$script:stopFile`"") -WindowStyle Hidden -PassThru
    $script:takeClock.Restart()
}
function Finish-Take {
    Mark 'Finished'
    $script:events | ConvertTo-Json | Set-Content "$script:Guide\$script:takeName-timeline.json"
    Start-Sleep -Seconds 3
    Set-Content -LiteralPath $script:stopFile -Value 'stop'
    while(!$script:record.HasExited){if($script:takeClock.Elapsed.TotalSeconds -gt ($script:takeLimit+15)){throw 'Capture deadline exceeded'};Start-Sleep -Milliseconds 500}
    Assert-Equal $script:record.ExitCode 0 'Final stereo recording completed'
    $meta=Get-Content "$script:Guide\$script:takeName-stereo.json" -Raw|ConvertFrom-Json
    $finished=($script:events|Where-Object action -eq 'Finished'|Select-Object -Last 1).seconds
    Assert-Equal ($meta.encodedFrames/$meta.fps -ge $finished) $true 'Recording covers the completed take'

}
function Stop-Take {
    try {Send-XR @('video off')} catch {}
    if($script:record -and !$script:record.HasExited){
        Set-Content -LiteralPath $script:stopFile -Value 'stop'
        if(!$script:record.WaitForExit(10000)){Stop-Process -Id $script:record.Id -Force}
    }
}
function Load-Guide {
    Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
    Send-Game @('load SC_QA_fixture') | Out-Null
    Start-Sleep -Seconds 4
    if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
    Park-Hands;Rest-Hands
}
