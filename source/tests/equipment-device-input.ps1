. "$PSScriptRoot/runtime-helpers.ps1"
$gameId=(Get-Content "$script:Session/state.json" -Raw|ConvertFrom-Json).pid
function Page {
    for($try=0;$try -lt 10;$try++){
        try{$p=Get-Content "$script:Session/wrist-settings-state.json" -Raw|ConvertFrom-Json;if($p.pid -eq $gameId){return $p}}catch{}
        Start-Sleep -Milliseconds 30
    }
}
function Grip([string]$hand){
    Send-XR "grip $hand 1"; Start-Sleep -Milliseconds 190
    Send-XR "grip $hand 0"; Start-Sleep -Milliseconds 380
}
function Push([int]$x,[int]$y){
    Send-XR "stick r $x $y"; Start-Sleep -Milliseconds 160
    Send-XR 'stick r center'; Start-Sleep -Milliseconds 270
}
function Button([string]$button){Send-XR "btn $button press 170"; Start-Sleep -Milliseconds 460}
function Category([string]$hand='r') {Send-XR 'trigger r .99'; Start-Sleep -Milliseconds 280; Grip $hand; Send-XR 'trigger r 0'; Start-Sleep -Milliseconds 450}
function Focus([string]$label){
    $p=Page
    $row=[array]::IndexOf(@($p.labels),$label)
    if($row -lt 0){throw "Missing $label on $($p.title)"}
    for($i=0;$i -lt $p.count -and (Page).selected -ne $row;$i++){Push 0 -1}
    Assert-Equal (Page).selected $row "Focus $label"
}
function Choose([string]$label){Focus $label;Button a}
function Shot([string]$name){
    if((Page).onArm -and !$script:filmingWrist){
        Send-XR @('head to 0 1.11 0 6 -32 0 500','hand l to grip .1695 1.0466 -.1768 -137.95 19.6 -167.1 550','hand r to grip .31 .74 -.32 5 36 9 500')
        Start-Sleep -Milliseconds 700
    }
    Save-Eyes "equipment-layout-$name"|Out-Host
    if((Page).onArm -and !$script:filmingWrist){
        Send-XR @('head to 0 1.11 0 8 -29 0 350','hand l to grip -.07 .97 -.45 -12 47 152 400','hand r to grip -.045 .96 -.28 0 -15 0 400')
        Start-Sleep -Milliseconds 600
    }
}
function Point-Click([double]$u,[double]$v){
    $frame=Get-Content "$script:Repo/evidence/equipment-pointer-frame.json" -Raw|ConvertFrom-Json
    $quad=@($frame.layers|Where-Object {$_.type -eq 'quad' -and $_.sizeM[0] -gt .55 -and $_.sizeM[1] -gt .40})[0]
    if(!$quad){throw 'Full Pip-Boy quad missing from actual compositor frame'}
    $q=$quad.orientation;$p=$quad.pose
    $x=($u-.5)*$quad.sizeM[0];$y=(.5-$v)*$quad.sizeM[1]
    $dx=$p[0]+$x*(1-2*($q[1]*$q[1]+$q[2]*$q[2]))+$y*2*($q[0]*$q[1]-$q[2]*$q[3])-.18
    $dy=$p[1]+$x*2*($q[0]*$q[1]+$q[2]*$q[3])+$y*(1-2*($q[0]*$q[0]+$q[2]*$q[2]))-.94
    $dz=$p[2]+$x*2*($q[0]*$q[2]-$q[1]*$q[3])+$y*2*($q[1]*$q[2]+$q[0]*$q[3])+.23
    $yaw=[Math]::Atan2(-$dx,-$dz)*180/[Math]::PI
    $pitch=[Math]::Atan2($dy,[Math]::Sqrt($dx*$dx+$dz*$dz))*180/[Math]::PI
    Send-XR @('hand r aimtrim 0 0',"hand r grip pose .18 .94 -.23 $yaw $pitch 0")
    Start-Sleep -Milliseconds 450
    Send-XR 'trigger r .96';Start-Sleep -Milliseconds 200;Send-XR 'trigger r 0';Start-Sleep -Milliseconds 420
}
function Start-Take([string]$name){
    $script:takePath=Join-Path "$script:Repo/evidence" "equipment-$gameId-$name"
    Send-XR 'video on'
    $script:recorder=Start-Process python.exe -WindowStyle Hidden -PassThru -ArgumentList @(
        ('"{0}/record-eyes.py"' -f $PSScriptRoot),'--pid',$gameId,'--eye','right','--fps',24,'--seconds',90,
        '--out',('"{0}.mp4"' -f $script:takePath),'--stop-file',('"{0}.stop"' -f $script:takePath))
    $deadline=[datetime]::UtcNow.AddSeconds(14)
    while(!(Test-Path "$script:takePath.ready.json")){
        if($script:recorder.HasExited -or [datetime]::UtcNow -gt $deadline){throw 'Final-eye recorder did not start'}
        Start-Sleep -Milliseconds 100
    }
}
function Finish-Take {
    New-Item -Path "$script:takePath.stop" -ItemType File | Out-Null
    if(!$script:recorder.WaitForExit(15000) -or $script:recorder.ExitCode -ne 0){throw 'Final-eye recorder failed'}
    Send-XR 'video off'
    $m=Get-Content "$script:takePath.json" -Raw|ConvertFrom-Json
    Assert-Equal $m.eye right 'Recorded the composed right eye'
    Assert-Equal (($m.encodedFrames-$m.repeatedFrames) -gt 80) $true 'Recorded distinct gameplay frames'
}
