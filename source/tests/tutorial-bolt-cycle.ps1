param([string]$Editor='WeapHuntingRifle',[string]$Handle='##HRBolt',[string]$TakeName='bolt-native-cycle',[switch]$Pump,[double]$GunYaw=35,[double]$ContactGrip=0,[switch]$CloseHold,[switch]$VideoOnly)
& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
. "$PSScriptRoot\tutorial-point-helper.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\tutorial-development';$script:events=[Collections.Generic.List[object]]::new()
Park-Hands
$weapon=Get-Native $Editor
Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250
Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon),'tgm')|Out-Null;Start-Sleep -Milliseconds 1200
Send-XR @("hand r grip pose .10 .90 -.50 $GunYaw 55 0",'hand l grip pose -.35 .65 -.15 0 55 0')
$ammo=Get-Native 'player.GetCurrentAmmo';Send-Game @(('player.AddItem {0:X8} 100' -f [uint32]$ammo))|Out-Null
if($CloseHold){
 Send-XR @('hand r curve grip .16 .84 -.38 0 65 -40 1600 0 .025 0','head to .18 1.1 .05 12 -20 0 1200');Start-Sleep -Milliseconds 1800
}else{Send-XR @('head to .25 1.1 .05 25 -25 0 1200');Start-Sleep -Milliseconds 1500}
Start-Take $TakeName 90
try{
 Mark $(if($Pump){'Fire, then point to the action'}else{'Fire, then point to the bolt'})
 Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 1000
 $a=Get-Addon;$a|ConvertTo-Json -Depth 22|Set-Content "$script:Repo\evidence\tutorial-$TakeName-before-state.json"
 Point-TutorialPart $Handle
 Send-XR @('grip l 0','touch l index on');Start-Sleep -Milliseconds 300
 Mark $(if($Pump){'Hold A with the gun hand and grip the action'}else{'Hold the off-hand trigger at the bolt'})
 for($i=0;$i -lt 3;$i++){
  $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$part=@($a.tutorial.parts|Where-Object node -eq $Handle)[0]
  $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$part.tracking[$j]-$a.nativeGripTracking[0][$j]}
  Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} -4 55 -6 1000 -.01 .025 0" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1200
 }
 $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$start=$s.handL.pos
 $part=@($a.tutorial.parts|Where-Object node -eq $Handle)[0];$end=@(0,0,0)
 for($j=0;$j -lt 3;$j++){$end[$j]=$start[$j]+$part.pullTracking[$j]-$part.tracking[$j]}
 if($Pump){Send-XR @('btn a down','grip l 1')}else{Send-XR @("grip l $ContactGrip",'trigger l 1')};Start-Sleep -Milliseconds 300
 Mark $(if($Pump){'Pull the action back'}else{'Pull the bolt back'})
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1500" -f $end[0],$end[1],$end[2]));Start-Sleep -Milliseconds 1700
 $back=Get-Addon
 $back|ConvertTo-Json -Depth 22|Set-Content "$script:Repo/evidence/tutorial-$TakeName-back-state.json"
 Assert-Equal $back.tutorial.step 'cycle-action' 'Guide does not claim ready while the action trigger is held'
 if(!$VideoOnly){Save-Eyes ("tutorial-"+$TakeName+"-back")}
 Mark $(if($Pump){'Return the action, then release grip and A'}else{'Return it and release the trigger'})
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1500" -f $start[0],$start[1],$start[2]));Start-Sleep -Milliseconds 1700
 Send-XR @('trigger l 0','grip l 0','btn a up','hand l curve grip -.30 .65 -.20 0 55 0 1300 -.03 .025 0');Start-Sleep -Milliseconds 1600
 $a=Get-Addon;$a|ConvertTo-Json -Depth 22|Set-Content "$script:Repo\evidence\tutorial-$TakeName-after-state.json"
 Assert-Equal $a.tutorial.chambered $true 'Native bolt cycle leaves a chambered round'
 $loaded=Get-Native 'player.GetCurrentAmmoRounds'
 Mark 'Fire the next round'
 Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 1000
 Assert-Equal ((Get-Native 'player.GetCurrentAmmoRounds') -lt $loaded) $true 'Native bolt cycle permits the next shot'
 if(!$VideoOnly){Save-Eyes ("tutorial-"+$TakeName+"-finished")};Finish-Take
}finally{Stop-Take}
