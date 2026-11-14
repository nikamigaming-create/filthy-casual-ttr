. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.6','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('click l press 180')
Start-Sleep -Milliseconds 350
Touch-Slot 0 0 | Out-Null
Start-Sleep -Milliseconds 600
foreach($pitch in 0,30,-30,55,-55){
    Send-XR @("hand r grip pose 0.20 1.38 -0.40 0 $pitch 0",'hand l grip pose -0.25 1.10 -0.28 0 0 0')
    Start-Sleep -Milliseconds 400
    Save-Eyes "pose-pitch-$pitch"
}
Send-XR @('hand r grip pose 0.20 1.38 -0.40 0 -30 0','video on')
$state=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
$record=Start-Process -FilePath 'python.exe' -ArgumentList @("`"$PSScriptRoot\record-eyes.py`"",'--pid',$state.pid,'--out',"`"$script:Repo\evidence\pose-probe.mp4`"",'--seconds',8,'--fps',30) -WindowStyle Hidden -PassThru
Start-Sleep -Milliseconds 500
Send-XR @('hand r to grip 0.12 1.43 -0.48 -12 -20 -8 1500')
Start-Sleep -Milliseconds 1600
Send-XR @('head to 0.02 1.60 0 8 -4 0 1200','hand r to grip 0.26 1.34 -0.35 15 -40 8 1800')
Start-Sleep -Milliseconds 1900
Send-XR @('head to 0 1.60 0 0 0 0 1600','hand r to grip 0.20 1.38 -0.40 0 -30 0 1600')
if(!$record.WaitForExit(15000)){Stop-Process -Id $record.Id;throw 'Recorder exceeded bounded take'}
Send-XR @('video off')
Assert-Equal $record.ExitCode 0 'Final-eye video recorder completed'
