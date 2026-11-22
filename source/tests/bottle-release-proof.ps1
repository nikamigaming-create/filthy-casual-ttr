. "$PSScriptRoot\runtime-helpers.ps1"
$stop="$script:Repo\work\vehicle-controls\bottle-release-proof.stop"
if(Test-Path -LiteralPath $stop){Remove-Item -LiteralPath $stop}
Send-XR @('video on')
$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
$rec=Start-Process python.exe -ArgumentList @("$PSScriptRoot\record-eyes.py",'--pid',$s.pid,'--out',"$script:Repo\work\vehicle-controls\bottle-release-proof.mp4",'--seconds',150,'--fps',30,'--stop-file',$stop) -WindowStyle Hidden -PassThru
try { & "$PSScriptRoot\bottle-handoff.ps1" }
finally {Set-Content -LiteralPath $stop -Value stop;$null=$rec.WaitForExit(12000);Send-XR @('video off')}
