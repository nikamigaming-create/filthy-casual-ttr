& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\tutorial-development';$script:events=[Collections.Generic.List[object]]::new()
$ammo=Get-Native 'player.GetCurrentAmmo'
Send-Game @(('player.AddItem {0:X8} 50' -f [uint32]$ammo))|Out-Null
$ammoList=Get-Native 'GetWeaponAmmo (player.GetEquippedObject 5)'
if((Get-Native 'GetType (GetWeaponAmmo (player.GetEquippedObject 5))') -eq 85){
 $variant=Get-Native 'ListGetNthForm (GetWeaponAmmo (player.GetEquippedObject 5)) 1'
 if($variant){Send-Game @(('player.AddItem {0:X8} 50' -f [uint32]$variant))|Out-Null}
}
Send-XR @('hand r to grip .42 .95 -.18 -35 55 0 1000','head to 0 1.1 0 0 -55 0 1200');Start-Sleep -Milliseconds 1500
Start-Take 'pouch-ammo-types' 50
try{
 Mark 'Look down at the ammo pouch'
 Start-Sleep -Seconds 2
 for($i=0;$i -lt 3;$i++){
  $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$s.handL.pos[$j]+$a.physicalPouchTracking[$j]-$a.nativeGripTracking[0][$j]}
  Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} -4 55 -6 1000 -.015 .02 0" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1200
 }
 Mark 'Hold the trigger at the pouch for one second'
 $before=Get-Addon
 Send-XR @('trigger l 1');Start-Sleep -Milliseconds 1600
 Send-XR @('trigger l 0');Start-Sleep -Milliseconds 800
 $after=Get-Addon;$selected=Get-Native 'player.GetCurrentAmmo'
 [ordered]@{before=$before;after=$after;beforeAmmo=$ammo;afterAmmo=$selected;variant=$variant}|ConvertTo-Json -Depth 25|Set-Content "$script:Repo\evidence\tutorial-ammo-types-state.json"
 Assert-Equal $selected $variant 'Native pouch trigger switches to the available alternate ammo type'
 Mark 'The ammunition type changes'
 Send-XR @('hand l to grip -.40 .65 -.18 -4 55 -6 1200');Start-Sleep -Milliseconds 1400
 Save-Eyes 'tutorial-ammo-types-open'
 Start-Sleep -Seconds 4
 Finish-Take
}finally{Stop-Take}
