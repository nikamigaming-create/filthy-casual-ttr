& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
. "$PSScriptRoot\tutorial-point-helper.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\tutorial-development';$script:events=[Collections.Generic.List[object]]::new()
Park-Hands
$weapon=Get-Native 'WeapNV44Revolver'
Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250
Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Milliseconds 1300
Send-XR @('hand r grip pose .10 .90 -.50 35 55 0','hand l grip pose -.35 .65 -.15 0 55 0')
$ammo=Get-Native 'player.GetCurrentAmmo';Send-Game @(('player.AddItem {0:X8} 100' -f [uint32]$ammo),'tgm')|Out-Null
for($i=0;$i -lt 10;$i++){if((Get-Native 'player.GetCurrentAmmoRounds') -eq 0){break};Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 900}
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 0 'Native firing empties revolver'
$closed=@((Get-Addon).tutorial.namedParts|Where-Object node -eq '##swmCrane')[0]
Start-Take 'cylinder-native-probe' 105
try{
 Mark 'Point at the cylinder'
 Point-TutorialPart '##swmCylinder'
 Send-XR @('grip l 0','hand l curve grip -.30 .65 -.20 0 55 0 1300 -.03 .025 0');Start-Sleep -Milliseconds 1500
 Mark 'Hold A and flick cylinder open'
 Send-XR @('btn a down');Start-Sleep -Milliseconds 150
 Send-XR @('hand r to grip .10 .90 -.50 35 55 75 250');Start-Sleep -Milliseconds 350
 Send-XR @('btn a up');Start-Sleep -Milliseconds 150
 Send-XR @('hand r to grip .10 .90 -.50 35 55 0 500');Start-Sleep -Milliseconds 800
 Mark 'Lower the muzzle and present the open cylinder'
 # Human reference: RevIndustries illustrated loader instructions, Figure C.
 # Bring the gun closer and lower; do not feed the loader through a level aim.
 Send-XR @('hand r curve grip .18 .62 -.32 15 15 0 1800 0 .025 0','head to 0 1.1 0 0 -45 0 1600');Start-Sleep -Milliseconds 2000
 $a=Get-Addon
 $target=@($a.tutorial.namedParts|Where-Object node -eq '##SpeedLoader')[0]
 $target|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\work\tutorial-dev\loader-target.json"
 Save-Eyes 'tutorial-cylinder-open'
 Mark 'Grip the speedloader from the pouch'
 for($i=0;$i -lt 3;$i++){
  $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$a.physicalPouchTracking[$j]-$a.nativeGripTracking[0][$j]}
  Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} -4 55 -6 1000 -.02 .025 0" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1150
 }
 Send-XR @('grip l 1');Start-Sleep -Milliseconds 600
 $a=Get-Addon;$a|ConvertTo-Json -Depth 20|Set-Content "$script:Repo\evidence\tutorial-cylinder-held-state.json"
 $a.pouch|ConvertTo-Json -Depth 5|Write-Output
 Save-Eyes 'tutorial-cylinder-held'
 Assert-Equal ($a.pouch.mode -gt 0) $true 'Native pouch grip acquires speedloader'
 Send-XR @('hand l curve grip -.25 .8 -.38 -4 55 -6 1400 -.03 .04 0');Start-Sleep -Milliseconds 1600
 Mark 'Aim the rounds into the open cylinder'
 for($i=0;$i -lt 3;$i++){
  $a=Get-Addon;$p=(& python "$PSScriptRoot\tutorial-loader-pose.py")|ConvertFrom-Json;$angles=$p.angles
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 1000" -f $s.handL.pos[0],$s.handL.pos[1],$s.handL.pos[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 1150
 }
 for($i=0;$i -lt 6;$i++){
  $a=Get-Addon;if((Get-Native 'player.GetCurrentAmmoRounds') -gt 0){break}
  $p=(& python "$PSScriptRoot\tutorial-loader-pose.py")|ConvertFrom-Json
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $next=@(0,0,0);for($j=0;$j -lt 3;$j++){$next[$j]=$s.handL.pos[$j]+$target.tracking[$j]-$p.held[$j]}
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 1200" -f $next[0],$next[1],$next[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 1400
 }
 $a=Get-Addon;$a|ConvertTo-Json -Depth 20|Set-Content "$script:Repo\evidence\tutorial-cylinder-insert-state.json"
 Assert-Equal ((Get-Native 'player.GetCurrentAmmoRounds') -gt 0) $true 'Native speedloader refills revolver'
 Send-XR @('grip l 0','hand l curve grip -.30 .65 -.20 0 55 0 1300 -.03 .025 0');Start-Sleep -Milliseconds 1500
 Save-Eyes 'tutorial-cylinder-insert'
 Mark 'Return to the normal hold and close the cylinder'
 Send-XR @('hand r curve grip .10 .90 -.50 35 55 0 1800 0 .025 0','head to .04 1.1 0 10 -20 0 1600');Start-Sleep -Milliseconds 2000

 Send-XR @('btn a down');Start-Sleep -Milliseconds 150
 Send-XR @('hand r to grip .10 .90 -.50 -30 55 0 180');Start-Sleep -Milliseconds 350
 Send-XR @('btn a up','hand r to grip .10 .90 -.50 35 55 0 800');Start-Sleep -Milliseconds 1000
 $a=Get-Addon;$after=@($a.tutorial.namedParts|Where-Object node -eq '##swmCrane')[0]
 $error=0;for($i=0;$i -lt 3;$i++){for($j=0;$j -lt 3;$j++){$error+=[Math]::Pow($closed.basis[$i][$j]-$after.basis[$i][$j],2)}}
 Assert-Equal ([Math]::Sqrt($error) -lt .05) $true 'Native cylinder returns to closed geometry'
 $a|ConvertTo-Json -Depth 20|Set-Content "$script:Repo\evidence\tutorial-cylinder-finished-state.json"
 Save-Eyes 'tutorial-cylinder-finished'
 Finish-Take
}finally{Stop-Take}

