. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('head to 0 1.1 0 0 -35 0 600')
$best=-2;$bestAngles='';$poses=@()
foreach($angles in @('0 0 0','0 0 90','0 0 -90','0 0 180','0 60 0','0 -60 0','0 60 180','0 -60 180','90 0 90','-90 0 90')){
 Send-XR @("hand r to grip 0.12 0.90 -0.35 $angles 350")
 Start-Sleep -Milliseconds 500
 $a=Get-Addon;$v=0..2|ForEach-Object {$a.wristEye[$_]-$a.wrist.origin[$_]}
 $len=[Math]::Sqrt(($v|ForEach-Object {$_*$_}|Measure-Object -Sum).Sum)
 $score=(0..2|ForEach-Object {$v[$_]*$a.wrist.normal[$_]}|Measure-Object -Sum).Sum/$len
 $poses+=@{angles=$angles;score=$score}
 if($score -gt $best){$best=$score;$bestAngles=$angles}
}
Write-Output "Best unobstructed cuff pose: $bestAngles score=$best"
Send-XR @("hand r to grip 0.12 0.90 -0.35 $bestAngles 700")
Start-Sleep -Milliseconds 900
Save-Eyes 'cuff-face-auto'
$a=Get-Addon;$initial=$a.manualReload
# Physical taps are proved by wrist-cuff.ps1. This isolates visible mode state.
Send-Game @('sc_reload')|Out-Null
Start-Sleep -Milliseconds 1400
Save-Eyes 'cuff-face-manual'
Send-Game @('sc_reload')|Out-Null
Assert-Equal (Get-Addon).manualReload $initial 'Visual comparison restores native reload setting'
Send-XR @('hand l to grip -0.20 0.95 -0.40 0 55 0 700','hand r to grip 0.30 0.70 -0.10 0 0 0 700')
Start-Sleep -Milliseconds 900
$fingers=@()
foreach($pose in @(@{name='open';grip=0;trigger=0;rest='off'},@{name='grip';grip=1;trigger=0;rest='off'},@{name='trigger';grip=0;trigger=1;rest='off'},@{name='thumbrest';grip=0;trigger=0;rest='on'})){
 Send-XR @("grip l $($pose.grip)","trigger l $($pose.trigger)","thumbrest l $($pose.rest)")
 Start-Sleep -Milliseconds 500
 $a=Get-Addon;$fingers+=@{pose=$pose.name;tips=$a.leftFingertips;hand=$a.hands[0]}
 Save-Eyes "native-fingers-$($pose.name)"
}
Send-XR @('grip l 0','trigger l 0','thumbrest l off')
@{poses=$poses;best=$bestAngles;fingerSamples=$fingers}|ConvertTo-Json -Depth 9|Set-Content "$script:Repo\evidence\wrist-views.json"
