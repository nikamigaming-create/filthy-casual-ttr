. "$PSScriptRoot\runtime-helpers.ps1"
function Controls {Send-Game @('sc_controls_state')|Out-Null;Get-Content "$script:Session\controls-state.json" -Raw|ConvertFrom-Json}
Send-XR @('capture off','pace free 60','input clear','profile touch','head pose 0 1.2 0 0 0 0','btn a press 120')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 120');Start-Sleep -Milliseconds 350}
Park-Hands
Send-Game @('sc_controls')|Out-Null
Start-Sleep -Milliseconds 500
$stop="$script:Repo\work\controller-profiles\montage.stop"
if(Test-Path -LiteralPath $stop){Remove-Item -LiteralPath $stop}
Send-XR @('video on')
$state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
$record=Start-Process python.exe -ArgumentList @("$PSScriptRoot\record-eyes.py",'--pid',$state.pid,'--out',"$script:Repo\work\controller-profiles\controllers-source.mp4",'--seconds',150,'--fps',30,'--stop-file',$stop) -WindowStyle Hidden -PassThru
$timeline=@();$clock=[Diagnostics.Stopwatch]::StartNew()
try {
 foreach($profile in 'plus','touch','pro','index','vive','wmr','pico','frame'){
  Send-XR @('input clear',"profile $profile",'head to 0 1.2 0 0 0 0 700','hand r to grip .42 .75 -.28 0 0 0 700')
  Start-Sleep -Milliseconds 1700
  $timeline+=@{profile=$profile;seconds=$clock.Elapsed.TotalSeconds}
  $c=Controls
  Assert-Equal $c.visible $true "$profile editor stays open"
  Assert-Equal $c.modelsReady $true "$profile renderer is available"
  $inputs=Get-Content "$script:Session\controls-profiles\inputs.json" -Raw|ConvertFrom-Json
  Assert-Equal ($inputs.controllers[0].profile -eq $c.devices[0]) $true "$profile detected-input dump matches live profile"
  Copy-Item "$script:Session\controls-profiles\inputs.json" "$script:Repo\evidence\inputs-$profile.json" -Force
  Send-XR @('grip l .8','grip r .8')
  Start-Sleep -Milliseconds 1000
  Assert-Equal ([double](Controls).physical[6] -gt .75) $true "$profile physical grip is routed"
  Send-XR @('grip l 0','grip r 0','trigger r .7')
  Start-Sleep -Milliseconds 1000
  Assert-Equal ([double](Controls).physical[5] -gt .65) $true "$profile analog trigger is routed"
  Save-Eyes "controllers-$profile"|Out-Host
  Send-XR @('trigger r 0')
  if($profile -eq 'frame'){
   foreach($source in 46,47,48,51,52,53,29){Send-XR @("source $source 1");Start-Sleep -Milliseconds 650;Assert-Equal ([double](Controls).physical[$source] -gt .9) $true "Frame extra source $source routed";Send-XR @("source $source 0");Start-Sleep -Milliseconds 250}
  }elseif($profile -eq 'index'){
   Send-XR @('source 45 .8');Start-Sleep -Milliseconds 700;Assert-Equal ([double](Controls).physical[45] -gt .75) $true 'Index trackpad force routed';Send-XR @('source 45 0')
  }
  Send-XR @('hand r to grip .35 1.45 -.45 0 0 0 1000')
  Start-Sleep -Milliseconds 1400
  Send-XR @('hand r to grip .42 .75 -.28 0 0 0 1000')
  Start-Sleep -Milliseconds 1000
 }
 Send-XR @('profile touch','input clear');Start-Sleep -Milliseconds 1400
 Send-XR @('btn b press 120');Start-Sleep -Milliseconds 400
 Assert-Equal (Controls).visible $false 'Editor closes after controller changes'
 Send-XR @('btn y press 120');Start-Sleep -Milliseconds 400
 Assert-Equal ((Get-Addon).menu -ne 0) $true 'Quest native Pause remains available'
 Send-XR @('btn b press 120');Start-Sleep -Milliseconds 400
 Assert-Equal (Get-Addon).menu 0 'Quest native Back remains available'
}finally{
 Set-Content -LiteralPath $stop -Value stop
 $null=$record.WaitForExit(12000)
 Send-XR @('video off')
 $timeline|ConvertTo-Json|Set-Content "$script:Repo\work\controller-profiles\montage-timeline.json"
}
Write-Output 'PASS controller profiles, extra buttons, model display, native Quest regression'
