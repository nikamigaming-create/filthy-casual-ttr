. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600}
Park-Hands
Send-Game @('player.SetWeaponOut 0') | Out-Null
Send-XR @('input clear','head to 0 1.1 0 0 -25 0 500','hand l to grip -0.20 0.95 -0.40 0 55 0 500','hand r to grip 0.20 0.95 -0.40 0 55 0 500')
Start-Sleep -Milliseconds 900
function Pair([string]$Left,[string]$Right,[int]$Hold=750){
 Send-XR @("btn $Left down","btn $Right down")
 Start-Sleep -Milliseconds $Hold
 Send-XR @("btn $Left up","btn $Right up")
 Start-Sleep -Milliseconds 250
}
function Back {Send-XR @('btn b press 100');Start-Sleep -Milliseconds 600}
$poses=@()
foreach($p in @(@{name='open';grip=0;trigger=0;rest='off';index='off'},@{name='rest';grip=0;trigger=0;rest='on';index='on'},@{name='point';grip=1;trigger=0;rest='on';index='off'},@{name='thumbsup';grip=1;trigger=0;rest='off';index='on'},@{name='fist';grip=1;trigger=0;rest='on';index='on'})){
 $lines=@();foreach($h in @('l','r')){$lines+="grip $h $($p.grip)";$lines+="trigger $h $($p.trigger)";$lines+="thumbrest $h $($p.rest)";$lines+="touch $h index $($p.index)"}
 Send-XR $lines;Start-Sleep -Milliseconds 700;$a=Get-Addon
 Assert-Equal $a.gestureGameplayAllowed $true "Empty hands permit $($p.name)"
 Assert-Equal (@($a.fingerGestures|Where-Object active).Count) 2 'Both Touch sensor sets active'
 $poses+=@{name=$p.name;hands=$a.fingerGestures}
 Save-Eyes "gesture-final-$($p.name)"
}
$poses|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\gesture-final-poses.json"
foreach($h in 0,1){
 $open=$poses[0].hands[$h];$rest=$poses[1].hands[$h];$point=$poses[2].hands[$h];$up=$poses[3].hands[$h];$fist=$poses[4].hands[$h]
 Assert-Equal ($rest.thumb -gt .33 -and $rest.thumb -lt .37) $true "Hand $h relaxed thumb between raised and fist"
 Assert-Equal ($rest.index -gt .18 -and $rest.index -lt .22) $true "Hand $h trigger touch is partial curl"
 Assert-Equal ($point.index -lt .02 -and $point.thumb -gt .73) $true "Hand $h independent pointing"
 Assert-Equal ($up.thumb -lt .02 -and $up.index -gt .98) $true "Hand $h independent thumbs up"
 Assert-Equal ($fist.thumb -gt .73 -and $fist.index -gt .98) $true "Hand $h full curl"
}
Send-XR @('input clear');Park-Hands
function Rest([string]$Hand,[int]$Ms=1150){
 Send-XR @('thumbrest l off','thumbrest r off');Start-Sleep -Milliseconds 120
 $other=if($Hand -eq "l"){"r"}else{"l"}
 Send-XR @("touch $other stick on","thumbrest $Hand on");Start-Sleep -Milliseconds $Ms
 Send-XR @("thumbrest $Hand off","touch $other stick off");Start-Sleep -Milliseconds 180
}
Assert-Equal (Get-Addon).wrist.attached $false 'No addon wrist cuff'
Rest 'l' 600
Assert-Equal (Get-Addon).openHand -1 'Short thumbrest contact cancels'
Send-XR @('thumbrest l on','thumbrest r on');Start-Sleep -Milliseconds 1250
Assert-Equal (Get-Addon).openHand -1 'Both resting thumbs do not open a wheel'
Send-XR @('thumbrest r off');Start-Sleep -Milliseconds 1150
Assert-Equal (Get-Addon).openHand -1 'Releasing one of two resting thumbs does not arm an old contact'
Rest 'l'
Assert-Equal (Get-Addon).openHand 0 'Left thumbrest hold opens consumables'
Assert-Equal (Get-Addon).menu 0 'Thumbrest hold does not open native menus'
Save-Eyes 'holds-consumables';Back
Rest 'r'
Assert-Equal (Get-Addon).openHand 1 'Right thumbrest hold opens weapons'
Save-Eyes 'holds-weapons';Back
$initial=(Get-Addon).manualReload
Send-XR @('btn menu press 150');Start-Sleep -Milliseconds 500
Assert-Equal (Get-Addon).manualReload (!$initial) 'Short left Menu tap changes reload once'
Assert-Equal (Get-Addon).menu 0 'Gameplay Menu tap does not trigger a native menu'
Send-XR @('btn menu down');Start-Sleep -Milliseconds 1150
Assert-Equal (Get-Addon).heightMenu $true 'One-second left Menu hold opens settings'
Assert-Equal (Get-Addon).manualReload (!$initial) 'Long hold does not also toggle reload'
Save-Eyes 'holds-settings'
Send-XR @('btn menu up');Start-Sleep -Milliseconds 200
Assert-Equal (Get-Addon).manualReload (!$initial) 'Release after long hold does not toggle reload'
Back
Send-XR @('btn menu press 150');Start-Sleep -Milliseconds 450
Assert-Equal (Get-Addon).manualReload $initial 'Second short Menu tap restores reload mode'
Send-XR @('touch r stick on','thumbrest l on');Start-Sleep -Milliseconds 600
Send-XR @('state visible');Start-Sleep -Milliseconds 500
Send-XR @('state focused');Start-Sleep -Milliseconds 1200
Assert-Equal (Get-Addon).openHand -1 'Interrupted thumbrest hold does not resume after focus loss'
Send-XR @('thumbrest l off','btn y press 100');Start-Sleep -Milliseconds 500
Assert-Equal ((Get-Addon).menu -ne 0) $true 'Y still opens native Pause without combo delay'
Send-XR @('btn b press 100');Start-Sleep -Milliseconds 500
Assert-Equal (Get-Addon).menu 0 'B still closes native Pause'
Write-Output 'PASS: finger poses, thumbrest holds, Menu tap/hold, cancellation, native Y/B'
