param([string]$TakeName="charger-clip-reload",[switch]$ClearContactView,[switch]$SideContactView,[switch]$AngledContactView)
& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
. "$PSScriptRoot\tutorial-point-helper.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\tutorial-development';$script:events=[Collections.Generic.List[object]]::new()
Send-Game @('tgm')|Out-Null
function State([string]$Tag){
 $a=Get-Addon;$a|ConvertTo-Json -Depth 22|Set-Content "$script:Repo\evidence\tutorial-exception-$Tag-state.json"
 Write-Output ("$Tag chamber=$($a.tutorial.chambered) rounds=$(Get-Native 'player.GetCurrentAmmoRounds') installed=$($a.pouch.installedMode) held=$($a.pouch.mode)")
}
function Equip([string]$Editor){
 Park-Hands;$w=Get-Native $Editor
 Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$w),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250
 Send-Game @(('sc_pick {0:X8}' -f [uint32]$w))|Out-Null;Start-Sleep -Milliseconds 1200
 Send-XR @('hand r grip pose .10 .90 -.50 35 55 0','hand l grip pose -.35 .65 -.15 0 55 0')
 $ammo=Get-Native 'player.GetCurrentAmmo';Send-Game @(('player.AddItem {0:X8} 100' -f [uint32]$ammo))|Out-Null
}
function Contact([string]$Node){
 for($i=0;$i -lt 3;$i++){
  $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $target=if($Node -eq 'pouch'){$a.physicalPouchTracking}else{@($a.tutorial.parts|Where-Object node -eq $Node)[0].tracking}
  $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$target[$j]-$a.nativeGripTracking[0][$j]}
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 700" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 850
 }
}
Equip 'WeapChinesePistol'
if($ClearContactView){
 # Reload at chest height: the clip grip sits about10cm above its model origin.
 # Holding the gun near eye height puts the large native wrist device over it.
 Send-XR @('head to .18 1.1 .02 12 -20 0 1400','hand r to grip .10 .72 -.45 35 55 0 1400');Start-Sleep -Milliseconds 1600
}
if($SideContactView){
 # A natural rightward lean exposes the guide beside the left wrist.
 Send-XR @('head to .40 1.10 .02 30 -28 0 1400');Start-Sleep -Milliseconds 1600
}
if($AngledContactView){
 # Expose the top guide beside the wrist by tilting the whole pistol naturally.
 Send-XR @('hand r to grip .10 .72 -.45 35 55 45 1400');Start-Sleep -Milliseconds 1600
}
for($i=0;$i -lt 20 -and (Get-Native 'player.GetCurrentAmmoRounds') -gt 0;$i++){Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 450}
State 'charger-empty'
Assert-Equal (Get-Addon).tutorial.card 'charger-guide' 'Charger guide does not ask for magazine ejection'
Start-Take $TakeName 120
try{
Mark 'Point to the slide; pull with the off-hand trigger'
Point-TutorialPart '##cmSlide'
Send-XR @('grip l 0','touch l index on');Start-Sleep -Milliseconds 300
Contact '##cmSlide'
Send-XR @('trigger l 1');Start-Sleep -Milliseconds 500
$a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$part=@($a.tutorial.parts|Where-Object node -eq '##cmSlide')[0]
$p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$part.pullTracking[$j]-$part.tracking[$j]}
Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1200" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1400
Mark 'While holding the slide back, tap A to lock it open'
Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400
Send-XR @('trigger l 0','hand l to grip -.3 .7 -.2 0 55 0 700');Start-Sleep -Milliseconds 900
State 'charger-locked';Save-Eyes 'tutorial-exception-charger-locked'
$target=@((Get-Addon).tutorial.namedParts|Where-Object node -eq '##cmClip')[0]
$target|Add-Member -NotePropertyName heldOccurrence -NotePropertyValue 1
$target|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\work\tutorial-dev\loader-target.json"

Mark 'Grip a loaded charger clip from the pouch'
Contact 'pouch';Send-XR @('grip l 1');Start-Sleep -Milliseconds 700
State 'charger-pouch';Save-Eyes 'tutorial-exception-charger-held'
Assert-Equal (Get-Addon).tutorial.card 'insert-charger' 'Held clip shows its own insertion instruction'
Mark 'Align the clip above the top loading guide'

Send-XR @('hand l to grip -.25 .80 -.38 -4 55 -6 1000');Start-Sleep -Milliseconds 1200
for($i=0;$i -lt 3;$i++){
 $p=(& python.exe "$PSScriptRoot\tutorial-loader-pose.py")|ConvertFrom-Json;$angles=$p.angles
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 1000" -f $s.handL.pos[0],$s.handL.pos[1],$s.handL.pos[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 1150
 State ("charger-align-"+$i)
}

$pose=(Get-Addon).tutorial.namedParts|Where-Object node -eq '##cmClip';$held=$pose[1]
$error=0;for($i=0;$i -lt 3;$i++){for($j=0;$j -lt 3;$j++){$error+=[Math]::Pow($target.basis[$i][$j]-$held.basis[$i][$j],2)}}
Assert-Equal ([Math]::Sqrt($error) -lt .06) $true 'Held charger aligns with native clip orientation'
# Approach the top along the clip's measured vertical axis, then feed downward.
# A 17cm overhead endpoint left the wrist above eye level after native transfer.
# Approach near the actual guide; keep the same authored orientation and controls.
$clearance=if($ClearContactView){.04}else{.17}
$top=@(0,0,0);for($j=0;$j -lt 3;$j++){$top[$j]=$target.tracking[$j]+$target.basis[1][$j]*$clearance}
for($i=0;$i -lt 3;$i++){
 # Native clip transfer can finish on the first approach. Never chase the
 # emptied/reattached clone after its ammunition leaves the hand.
 $current=Get-Addon;if($current.pouch.installedRounds -gt 0 -or $current.pouch.heldRounds -le 0){break}
 $p=(& python.exe "$PSScriptRoot\tutorial-loader-pose.py")|ConvertFrom-Json
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $next=@(0,0,0);for($j=0;$j -lt 3;$j++){$next[$j]=$s.handL.pos[$j]+$top[$j]-$p.held[$j]}
 $reach=0;for($j=0;$j -lt 3;$j++){$reach+=[Math]::Pow($next[$j]-$s.head.pos[$j],2)}
 if([Math]::Sqrt($reach) -gt .95){throw 'Rejected choreography: hand exceeds seated reach'}
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F3} {3:F3} {4:F3} {5:F3} 1100" -f $next[0],$next[1],$next[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 1300
}
Mark 'Seat the clip and release grip'
State 'charger-above';Save-Eyes 'tutorial-exception-charger-above'
for($i=0;$i -lt 5;$i++){

 if((Get-Addon).pouch.installedRounds -gt 0){break}
 $p=(& python.exe "$PSScriptRoot\tutorial-loader-pose.py")|ConvertFrom-Json
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $next=@(0,0,0);for($j=0;$j -lt 3;$j++){$next[$j]=$s.handL.pos[$j]+$target.tracking[$j]-$p.held[$j]}
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 1200" -f $next[0],$next[1],$next[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 1400
 State ("charger-insert-"+$i)
}
State 'charger-port';Save-Eyes 'tutorial-exception-charger-port'
Send-XR @('grip l 0','trigger l 0','btn a up','hand l to grip -.3 .7 -.2 0 55 0 700');Start-Sleep -Milliseconds 900

Assert-Equal (Get-Addon).pouch.installedRounds 10 'Native charger feeds ten rounds into the internal magazine'
Mark 'Point to the slide, then pull and release to chamber'
Point-TutorialPart '##cmSlide'
Send-XR @('grip l 0','touch l index on');Start-Sleep -Milliseconds 300
Contact '##cmSlide'
Send-XR @('trigger l 1');Start-Sleep -Milliseconds 500
$a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$part=@($a.tutorial.parts|Where-Object node -eq '##cmSlide')[0]
$p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$part.pullTracking[$j]-$part.tracking[$j]}
Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1200" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1400
Send-XR @('trigger l 0','hand l to grip -.3 .7 -.2 0 55 0 900');Start-Sleep -Milliseconds 1200
State 'charger-closed';Save-Eyes 'tutorial-exception-charger-closed'
Assert-Equal (Get-Addon).tutorial.chambered $true 'Native slide closes and chambers a round after charger loading'
Assert-Equal (Get-Addon).tutorial.card 'charger-ready' 'Loaded charger weapon has a correct ready instruction'
Mark 'Fire the loaded pistol'
$loaded=(Get-Addon).pouch.installedRounds
Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 900
State 'charger-shot'
Assert-Equal ((Get-Addon).pouch.installedRounds -lt $loaded) $true 'Charger reload permits a real follow-up shot'

Finish-Take
}finally{Stop-Take}
