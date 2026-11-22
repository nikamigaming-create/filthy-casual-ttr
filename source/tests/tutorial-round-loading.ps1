param([string[]]$Editors=@('WeapNVCaravanShotgun','WeapNV357Revolver','WeapNVHuntingShotgun'))
& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
. "$PSScriptRoot\tutorial-point-helper.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\tutorial-development'
Send-Game @('tgm')|Out-Null
function Move-GripTo([string]$Target){
 for($i=0;$i -lt 3;$i++){
  $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $t=if($Target -eq 'pouch'){$a.physicalPouchTracking}else{@($a.tutorial.parts|Where-Object node -eq $Target)[0].tracking}
  if($t.Count -ne 3){throw "Missing real interaction target: $Target"}
  $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$t[$j]-$a.nativeGripTracking[0][$j]}
  Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} -4 55 -6 1100 -.015 .025 0" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1250
 }
}

function Invoke-NativePumpCycle {
 Mark 'Cycle the pump with the native controls'
 Point-TutorialPart '##Stock';Send-XR @('grip l 0','touch l index on');Start-Sleep -Milliseconds 300
 for($i=0;$i -lt 3;$i++){
  $a=Get-Addon;$part=@($a.tutorial.parts|Where-Object node -eq '##Stock')[0]
  $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$state.handL.pos[$j]+$part.tracking[$j]-$a.nativeGripTracking[0][$j]}
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 900" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1050
 }
 $a=Get-Addon;$part=@($a.tutorial.parts|Where-Object node -eq '##Stock')[0]
 $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$start=$state.handL.pos
 $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$start[$j]+$part.pullTracking[$j]-$part.tracking[$j]}
 Send-XR @('btn a down','grip l 1');Start-Sleep -Milliseconds 300
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1500" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1700
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1500" -f $start[0],$start[1],$start[2]));Start-Sleep -Milliseconds 1700
 Send-XR @('grip l 0','btn a up','hand l curve grip -.30 .65 -.20 0 55 0 1200 -.02 .025 0');Start-Sleep -Milliseconds 1400
 Assert-Equal (Get-Addon).tutorial.chambered $true 'Native pump cycle chambers a round'
}

foreach($editor in $Editors){
 Park-Hands
 $weapon=Get-Native $editor
 Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 200
 Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Milliseconds 1200
 Send-XR @('hand r grip pose .10 .90 -.50 35 55 0','hand l grip pose -.35 .65 -.15 0 55 0')
 $ammo=Get-Native 'player.GetCurrentAmmo';Send-Game @(('player.AddItem {0:X8} 100' -f [uint32]$ammo))|Out-Null
 $before=Get-Native 'player.GetCurrentAmmoRounds';Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 1000
 $empty=Get-Native 'player.GetCurrentAmmoRounds'
 Assert-Equal ($empty -lt $before) $true "$editor native shot makes room"
 $script:events=[Collections.Generic.List[object]]::new()
 Start-Take ("round-load-"+$editor) 100
 try{
  if($editor -eq 'WeapNVHuntingShotgun' -and !(Get-Addon).tutorial.chambered){
   Invoke-NativePumpCycle
  }
  Mark 'Point to the loading port'
  Point-TutorialPart 'loading-port'
  Send-XR @('grip l 0','hand l curve grip -.30 .65 -.20 0 55 0 1200 -.02 .025 0');Start-Sleep -Milliseconds 1400
  if($editor -ne 'WeapNVHuntingShotgun'){
   Mark 'Open the breech or gate'
   Send-XR @('btn a down');Start-Sleep -Milliseconds 120
   $motion=if($editor -in @('WeapNVCaravanShotgun','WeapNVGrenadeRifle')){'hand r to grip .10 .90 -.50 35 -10 0 180'}else{'hand r to grip .10 .90 -.50 35 55 75 180'}
   Send-XR @($motion);Start-Sleep -Milliseconds 300
   Send-XR @('btn a up','hand r to grip .10 .90 -.50 35 55 0 600');Start-Sleep -Milliseconds 800
  }
  Save-Eyes ("tutorial-round-"+$editor+'-open')
  if($editor -eq 'WeapNVHuntingShotgun'){
   Mark 'Bring the loading port into view'
   # Human reference: Mossberg loading demonstration, 2:21. Present the
   # underside near the torso instead of reaching around a level aiming pose.
   Send-XR @('hand r curve grip .14 .78 -.38 35 55 0 1600 0 .025 0');Start-Sleep -Milliseconds 1800
   $null=Get-Addon;$roll=(& python.exe "$PSScriptRoot\tutorial-barrel-roll.py" 100)|ConvertFrom-Json
   Send-XR @(("hand r curve grip .14 .78 -.38 {0:F3} {1:F3} {2:F3} 1800 0 .02 0" -f $roll.angles[0],$roll.angles[1],$roll.angles[2]));Start-Sleep -Milliseconds 2000
   Save-Eyes 'tutorial-pump-barrel-roll'
  }
  Mark 'Grip one round from the pouch'
  Move-GripTo 'pouch';Send-XR @('grip l 1');Start-Sleep -Milliseconds 500
  $a=Get-Addon;$a|ConvertTo-Json -Depth 22|Set-Content "$script:Repo\evidence\tutorial-round-$editor-held-state.json"
  Assert-Equal ($a.pouch.mode -gt 0) $true "$editor native pouch provides loose ammunition"
  Send-XR @('hand l curve grip -.25 .8 -.38 -4 55 -6 1400 -.03 .04 0');Start-Sleep -Milliseconds 1600
  Mark 'Bring the round to the actual loading port'
  $beforeInsert=Get-Native 'player.GetCurrentAmmoRounds'
  if($editor -in @('WeapNVHuntingShotgun','WeapNVGrenadeRifle')){
   # Align the actual held shell with the authored chambered-round frame.
   # Moving only the wrist to the port is insufficient when the gun is tilted.
   $target=@((Get-Addon).tutorial.namedParts|Where-Object node -eq '##Shells')[0]
   $target|Add-Member -NotePropertyName heldOccurrence -NotePropertyValue 1
   $target|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\work\tutorial-dev\loader-target.json"
   for($i=0;$i -lt 3;$i++){
    $null=Get-Addon;$pose=(& python.exe "$PSScriptRoot\tutorial-loader-pose.py")|ConvertFrom-Json;$angles=$pose.angles
    $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
    Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 900" -f $state.handL.pos[0],$state.handL.pos[1],$state.handL.pos[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 1100
   }
   for($i=0;$i -lt 4;$i++){
    $a=Get-Addon;if((Get-Native 'player.GetCurrentAmmoRounds') -gt $beforeInsert){break}
    $pose=(& python.exe "$PSScriptRoot\tutorial-loader-pose.py")|ConvertFrom-Json
    $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
    $port=@($a.tutorial.parts|Where-Object node -eq 'loading-port')[0].tracking
    $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$state.handL.pos[$j]+$port[$j]-$pose.held[$j]}
    Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 1200 -.01 .02 0" -f $p[0],$p[1],$p[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 1400
   }
   if($editor -eq 'WeapNVHuntingShotgun'){
    Mark 'Seat shell at the native loading point'
    for($push=0;$push -lt 3;$push++){
     $a=Get-Addon;if((Get-Native 'player.GetCurrentAmmoRounds') -gt $beforeInsert){break}
     Assert-Equal $a.pouch.roundTargetValid $true 'Native tube target is readable'
     $contact=$a.nativeGripTracking[0]
     if($a.pouch.heldBoundValid){
      $d=0;for($j=0;$j -lt 3;$j++){$d+=[Math]::Pow($a.pouch.heldBoundTracking[$j]-$contact[$j],2)}
      if($d -lt .25*.25){$contact=$a.pouch.heldBoundTracking}
     }
     $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
     $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$state.handL.pos[$j]+$a.pouch.roundTargetTracking[$j]-$contact[$j]}
     Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 1100" -f $p[0],$p[1],$p[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 1300
    }
   }
  }else{Move-GripTo 'loading-port';Start-Sleep -Milliseconds 600}
  $a=Get-Addon;$a|ConvertTo-Json -Depth 22|Set-Content "$script:Repo\evidence\tutorial-round-$editor-insert-state.json"
  Save-Eyes ("tutorial-round-"+$editor+'-insert')
  Assert-Equal ((Get-Native 'player.GetCurrentAmmoRounds') -gt $beforeInsert) $true "$editor native round insertion increases ammunition"
  Send-XR @('grip l 0','hand l curve grip -.30 .65 -.20 0 55 0 1300 -.03 .025 0');Start-Sleep -Milliseconds 1500
  if($editor -eq 'WeapNVHuntingShotgun'){
   Mark 'Return to the normal hold'
   Send-XR @('hand r curve grip .10 .90 -.50 35 55 0 1800 0 .025 0');Start-Sleep -Milliseconds 2000
  }
  if($editor -ne 'WeapNVHuntingShotgun'){
   Mark 'Close the action with the native wrist motion'
   Send-XR @('btn a down');Start-Sleep -Milliseconds 120
   $motion=if($editor -in @('WeapNVCaravanShotgun','WeapNVGrenadeRifle')){'hand r to grip .10 .90 -.50 35 115 0 180'}else{'hand r to grip .10 .90 -.50 -30 55 0 180'}
   Send-XR @($motion);Start-Sleep -Milliseconds 300
   Send-XR @('btn a up','hand r to grip .10 .90 -.50 35 55 0 600');Start-Sleep -Milliseconds 900
  }
  $loaded=Get-Native 'player.GetCurrentAmmoRounds'
  if($editor -eq 'WeapNVHuntingShotgun'){Assert-Equal (Get-Addon).tutorial.card 'pump-ready' 'Chambered topped-up pump says ready, not cycle again'}
  Mark 'Fire the newly loaded ammunition'
  Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 1000
  Assert-Equal ((Get-Native 'player.GetCurrentAmmoRounds') -lt $loaded) $true "$editor native reload produces a usable shot"
  $a=Get-Addon;$a|ConvertTo-Json -Depth 22|Set-Content "$script:Repo\evidence\tutorial-round-$editor-finished-state.json"
  Save-Eyes ("tutorial-round-"+$editor+'-finished')
  Finish-Take
 }finally{Stop-Take}
}
