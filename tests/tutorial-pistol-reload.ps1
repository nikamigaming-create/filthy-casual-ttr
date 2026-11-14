param([switch]$Dry,[string]$Editor='Weap10mmPistol',[string]$TakeName='',[switch]$StageReady,[switch]$SkipPointing,[double[]]$PouchTarget=@())
$ErrorActionPreference='Stop'
if(!$StageReady){& "$PSScriptRoot\tutorial-visual-smoke.ps1"}
. "$PSScriptRoot\casual-guide-helpers.ps1"
. "$PSScriptRoot\tutorial-point-helper.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\tutorial-development'
$script:events=[Collections.Generic.List[object]]::new()
if($Editor -ne 'Weap10mmPistol'){
 Park-Hands
 $weapon=Get-Native $Editor
 Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250
 Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Milliseconds 1200
}
Send-XR @('hand r curve grip .10 .90 -.50 35 55 0 1600 0 .03 0');Start-Sleep -Milliseconds 1800

$ammo=Get-Native 'player.GetCurrentAmmo'
Send-Game @(('player.AddItem {0:X8} 100' -f [uint32]$ammo))|Out-Null
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 500}
$a=Get-Addon
$magNode=$a.tutorial.parts[0].node
$handleNode=if($a.tutorial.parts.Count -gt 1){$a.tutorial.parts[1].node}else{''}
Assert-Equal ($null -ne $magNode) $true 'Native manual magazine metadata is ready'
function Record-Step([string]$Name){
 $a=Get-Addon
 $a|ConvertTo-Json -Depth 15|Set-Content "$script:Repo\evidence\tutorial-$take-$Name-state.json"
 $a.tutorial|ConvertTo-Json -Compress -Depth 5|Write-Output
 Save-Eyes "tutorial-$take-$Name"
 return $a
}
if($Dry){
 # Prepare an actually empty gun using native firing, before the teaching take.
 Send-Game @('tgm')|Out-Null
 for($shot=0;$shot -lt 64;$shot++){
  if((Get-Native 'player.GetCurrentAmmoRounds') -eq 0){break}
  Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 400
 }
 Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 0 'Native shots emptied pistol'
 $a=Get-Addon;$a|ConvertTo-Json -Depth 15|Set-Content "$script:Repo\evidence\tutorial-after-emptying.json"
}
$take=if($TakeName){$TakeName}elseif($Dry){'pistol-dry-reload'}else{'pistol-reload-prototype'}
Start-Take $take 115
try{
 Mark 'Point at magazine'
 if(!$SkipPointing){Point-TutorialPart $magNode}
 Send-XR @('grip l 0','hand l curve grip -.30 .75 -.25 0 55 0 1200 -.03 .025 0')
 Start-Sleep -Milliseconds 1400
 Mark 'Eject using native A'
 Send-XR @('btn a press 180');Start-Sleep -Seconds 2
 $a=Get-Addon
 Assert-Equal $a.pouch.installedMode 4 'Native A leaves empty well'
 Assert-Equal $a.tutorial.step 'fetch-ammo' 'Guide asks for pouch ammunition'
 Record-Step 'empty'|Out-Null
 Mark 'Reach to pouch and grip spare'
 for($i=0;$i -lt 3;$i++){
  $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $t=if($PouchTarget.Count -eq 3){$PouchTarget}elseif($a.insertion.pouchValid){$a.insertion.pouchTarget}else{$a.pouch.heldBoundTracking};$c=$a.nativeGripTracking[0]
  if($t.Count -ne 3){throw 'Native pouch tracking target unavailable'}
  $p=@(0,0,0);if($PouchTarget.Count -eq 3){$p=@($t[0],($t[1]-$a.heightOffset),$t[2])}else{for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$t[$j]-$c[$j]}}
  Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} -4 55 -6 1200 -.02 .035 0" -f $p[0],$p[1],$p[2]))
  Start-Sleep -Milliseconds 1350
 }
 Send-XR @('grip l 1');Start-Sleep -Milliseconds 700
 $a=Get-Addon
 $a|ConvertTo-Json -Depth 15|Set-Content "$script:Repo\evidence\tutorial-reload-pouch-grip.json"
 Assert-Equal $a.pouch.installedMode 2 'Native grip acquires spare into insertion state'
 Assert-Equal $a.tutorial.step 'insert-magazine' 'Guide advances on actual native spare pickup'
 Send-XR @('hand l curve grip -.28 .80 -.32 -4 55 -6 1400 -.03 .04 0');Start-Sleep -Milliseconds 1600
 Mark 'Align spare with the well'
 for($align=0;$align -lt 3;$align++){
  $a=Get-Addon;$angles=(& python.exe "$PSScriptRoot\magazine-pose.py" align)|ConvertFrom-Json
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 900" -f $s.handL.pos[0],$s.handL.pos[1],$s.handL.pos[2],$angles[0],$angles[1],$angles[2]))
  Start-Sleep -Milliseconds 1050
 }
 Record-Step 'held'|Out-Null
 Mark 'Seat magazine through native insertion'
 for($i=0;$i -lt 8;$i++){
  $a=Get-Addon;if($a.pouch.installedMode -eq 0){break}
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $t=$a.insertion.target;$c=$a.insertion.held;$p=@(0,0,0)
  for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$t[$j]-$c[$j]}
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 1000" -f $p[0],$p[1],$p[2],$angles[0],$angles[1],$angles[2]))
  Start-Sleep -Milliseconds 1150
 }
 Send-XR @('grip l 0');Start-Sleep -Milliseconds 700
 $a=Get-Addon
 $a|ConvertTo-Json -Depth 15|Set-Content "$script:Repo\evidence\tutorial-reload-result.json"
 Assert-Equal $a.pouch.installedMode 0 'Native spare seated with tutorial on'
 if($Dry){
  Assert-Equal $a.tutorial.step 'cycle-action' 'Guide asks to rack after empty-gun reload'
  Assert-Equal $a.tutorial.chambered $false 'Inserted rounds require native chambering'
 }else{Assert-Equal $a.tutorial.step 'ready' 'Guide returns to ready after native reload'}
 Send-XR @('hand l curve grip -.30 .75 -.25 0 55 0 1400 -.03 .025 0');Start-Sleep -Milliseconds 1600
 if($Dry){
  Mark 'Point at the slide before racking'
  Point-TutorialPart $handleNode
  Send-XR @('grip l 0','touch l index on');Start-Sleep -Milliseconds 400
  Mark 'Contact slide with off-hand trigger'
  for($i=0;$i -lt 3;$i++){
   $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
   $part=@($a.tutorial.parts|Where-Object node -eq $handleNode)[0]
   $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$part.tracking[$j]-$a.nativeGripTracking[0][$j]}
   Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} -4 55 -6 1000 -.01 .025 0" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1150
  }
  $a=Get-Addon;$beforeSlide=@($a.tutorial.parts|Where-Object node -eq $handleNode)[0].nodeTracking
  Send-XR @('trigger l 1');Start-Sleep -Milliseconds 500
  $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $part=@($a.tutorial.parts|Where-Object node -eq $handleNode)[0]
  $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$part.pullTracking[$j]-$part.tracking[$j]}
  Mark 'Pull slide back, then release trigger'
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1500" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1700
  Record-Step 'slide-back'|Out-Null
  $a=Get-Addon;$afterSlide=@($a.tutorial.parts|Where-Object node -eq $handleNode)[0].nodeTracking
  $moved=0;for($j=0;$j -lt 3;$j++){$moved+=[Math]::Pow($afterSlide[$j]-$beforeSlide[$j],2)}
  Write-Output ("Native slide movement: {0:F5}m" -f [Math]::Sqrt($moved))
  # Parent-node position can return before the pull finishes; actual chamber
  # transition and recorded final-eye motion are the mechanism evidence.
  Send-XR @('trigger l 0','hand l curve grip -.30 .75 -.25 0 55 0 1400 -.03 .025 0');Start-Sleep -Milliseconds 1800
  $a=Get-Addon
  $a|ConvertTo-Json -Depth 15|Set-Content "$script:Repo\evidence\tutorial-chamber-result.json"
  Assert-Equal $a.tutorial.chambered $true 'Native trigger and slide motion chamber a round'
  Assert-Equal $a.tutorial.step 'ready' 'Guide observes completed native chambering'
 }
 Record-Step 'finished'|Out-Null
 Finish-Take
}finally{Stop-Take}
