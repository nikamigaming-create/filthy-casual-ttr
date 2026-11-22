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
foreach($p in @(@{name='open';grip=0;trigger=0;rest='off';index='off'},@{name='rest';grip=0;trigger=0;rest='on';index='on'},@{name='point';grip=1;trigger=0;rest='on';index='off'},@{name='thumbsup';grip=1;trigger=1;rest='off';index='on'},@{name='fist';grip=1;trigger=1;rest='on';index='on'})){
 $lines=@();foreach($h in @('l','r')){$lines+="grip $h $($p.grip)";$lines+="trigger $h $($p.trigger)";$lines+="thumbrest $h $($p.rest)";$lines+="touch $h index $($p.index)"}
 Send-XR $lines;Start-Sleep -Milliseconds 700;$a=Get-Addon
 Assert-Equal $a.gestureHandsFree $true "Empty hands permit $($p.name)"
 Assert-Equal (@($a.fingerGestures|Where-Object active).Count) 2 'Both Touch sensor sets active'
 $poses+=@{name=$p.name;hands=$a.fingerGestures}
 Save-Eyes "gesture-final-$($p.name)"
}
$poses|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\gesture-final-poses.json"
foreach($h in 0,1){
 $open=$poses[0].hands[$h];$rest=$poses[1].hands[$h];$point=$poses[2].hands[$h];$up=$poses[3].hands[$h];$fist=$poses[4].hands[$h]
 Assert-Equal ($rest.thumb -gt .43 -and $rest.thumb -lt .47) $true "Hand $h relaxed thumb between raised and fist"
 Assert-Equal ($rest.index -gt .23 -and $rest.index -lt .27) $true "Hand $h trigger touch is partial curl"
 Assert-Equal ($point.index -lt .02 -and $point.thumb -gt .98) $true "Hand $h independent pointing"
 Assert-Equal ($up.thumb -lt .02 -and $up.index -gt .98) $true "Hand $h independent thumbs up"
 Assert-Equal ($fist.thumb -gt .98 -and $fist.index -gt .98) $true "Hand $h full curl"
}
Send-XR @('input clear');Park-Hands
Assert-Equal (Get-Addon).wrist.attached $false 'No addon wrist cuff attached'
Pair 'x' 'b' 180
Assert-Equal (Get-Addon).openHand -1 'Short combo cancels without selector'
Assert-Equal (Get-Addon).menu 0 'Short combo does not trigger native menu'
Pair 'x' 'a'
Assert-Equal (Get-Addon).openHand 1 'Held X+A opens weapons'
Assert-Equal (Get-Addon).menu 0 'Weapon chord does not trigger native activate/menu'
Back
Assert-Equal (Get-Addon).openHand -1 'Standalone B closes selector'
Pair 'x' 'b'
Assert-Equal (Get-Addon).openHand 0 'Held X+B opens consumables'
Assert-Equal (Get-Addon).menu 0 'Consumable chord does not open VATS'
Back
Pair 'y' 'b'
Assert-Equal (Get-Addon).heightMenu $true 'Held Y+B opens height settings'
Assert-Equal (Get-Addon).menu 0 'Height chord does not open Pause/VATS'
Back
$initial=(Get-Addon).manualReload
Pair 'y' 'a' 1200
Assert-Equal (Get-Addon).manualReload (!$initial) 'Held Y+A switches reload exactly once'
Assert-Equal (Get-Addon).menu 0 'Reload chord does not open Pause'
Pair 'y' 'a'
Assert-Equal (Get-Addon).manualReload $initial 'Second deliberate reload chord restores mode'
Send-XR @('btn y press 100');Start-Sleep -Milliseconds 600
Assert-Equal ((Get-Addon).menu -ne 0) $true 'Standalone Y still opens native Pause'
Send-XR @('btn b press 100');Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).menu 0 'Standalone B closes native Pause'
Write-Output 'PASS: fingers, all four held chords, cancellation and native Pause'
