. "$PSScriptRoot\casual-guide-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600}
Park-Hands
Send-Game @('player.SetWeaponOut 0') | Out-Null
Send-XR @('input clear','head to 0 1.1 0 0 -25 0 500','hand l to grip -0.20 0.95 -0.40 0 55 0 500','hand r to grip 0.20 0.95 -0.40 0 55 0 500')
Start-Sleep -Milliseconds 900
$poses=@()
foreach($p in @(@{name='open';grip=0;rest='off';index='off'},@{name='relaxed';grip=0;rest='on';index='on'},@{name='fist';grip=1;rest='on';index='on'},@{name='thumbsup';grip=1;rest='off';index='on'},@{name='point';grip=1;rest='on';index='off'})){
 $lines=@();foreach($h in @('l','r')){$lines+="grip $h $($p.grip)";$lines+="trigger $h 0";$lines+="thumbrest $h $($p.rest)";$lines+="touch $h index $($p.index)"}
 Send-XR $lines;Start-Sleep -Milliseconds 850;$a=Get-Addon
 Assert-Equal (@($a.fingerGestures|Where-Object active).Count) 2 "Both sensor sets active: $($p.name)"
 if($p.name -in @('fist','thumbsup')){foreach($h in 0,1){Assert-Equal ($a.fingerGestures[$h].index -gt .98) $true "Hand $h $($p.name) closes index without trigger pull"}}
 if($p.name -eq 'fist'){foreach($h in 0,1){Assert-Equal ($a.fingerGestures[$h].thumb -gt .98) $true "Hand $h full grip reaches the authored closed thumb";Assert-Equal ($a.fingerGestures[$h].lift -lt .02) $true "Hand $h fist has no thumb lift"}}
 if($p.name -eq 'thumbsup'){foreach($h in 0,1){Assert-Equal ($a.fingerGestures[$h].lift -gt .98) $true "Hand $h thumb raised"}}
 $poses+=@{name=$p.name;hands=$a.fingerGestures};Save-Eyes "natural-$($p.name)"
}
$poses|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\natural-poses.json"
Write-Output 'PASS: final relaxed, fist, raised thumb and pointing pose checks'
