param([uint32]$Weapon=0x8F21E,[int]$Rounds=13,[string]$Label="rifle",[switch]$Tutorial)
$SaveReload=$false;$Gravity=$true
. "$PSScriptRoot\magazine-helpers.ps1"
. "$PSScriptRoot\magazine-gravity.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
$script:Guide=Join-Path $script:Repo $(if($Tutorial){'outputs\tutorial-development'}else{'outputs\magazine-recovery'})
New-Item -ItemType Directory -Force $script:Guide|Out-Null
$script:events=[Collections.Generic.List[object]]::new()
if($Tutorial){
 & "$PSScriptRoot\tutorial-visual-smoke.ps1"
 # The visual fixture enables god mode; disable it before ammo-conservation proof.
 Send-Game @('tgm')|Out-Null
}else{
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -25 0','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
}
$recoverBefore=(Get-Addon).physicalMags.recovered
Park-Hands
if(!$Tutorial -or (Get-Addon).tutorial.weapon -ne $Weapon){
Send-Game @(('player.AddItem {0:X8} 1' -f $Weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @(('sc_pick {0:X8}' -f $Weapon))|Out-Null;Start-Sleep -Seconds 1
}
Assert-Equal (Get-Addon).weaponHand 1 'Recovery demonstration starts with the weapon actually held'
if((Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 300}
$ammo=Get-Native 'GetWeaponAmmo (player.GetEquippedObject 5)'
if((Get-Native 'GetType (GetWeaponAmmo (player.GetEquippedObject 5))') -eq 85){$ammo=Get-Native 'ListGetNthForm (GetWeaponAmmo (player.GetEquippedObject 5)) 0'}
Send-Game @(('player.AddItem {0:X8} 100' -f $ammo),"player.SetCurrentAmmoRounds $Rounds")|Out-Null;Start-Sleep -Milliseconds 600
$inventory=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
Send-Game @('sc_reload')|Out-Null
Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0','hand l grip pose -0.5 0.75 -0.3 0 30 0');Start-Sleep -Seconds 1
Start-Take $Label 110
try {
Mark 'Manual magazine ejection'
Start-Sleep -Seconds 2
Send-XR @('btn a press 180');Start-Sleep -Seconds 2
$a=Get-Addon;$partId=$a.physicalMags.lastRef;$part=@($a.physicalParts|Where-Object ref -eq $partId)[0]
Assert-Equal ($partId -gt 0) $true 'Native eject creates a real world magazine'
Assert-Equal $a.pouch.installedMode 4 'Magazine well is empty after ejection'
$worldRounds=if($Rounds){$part.stackCount}else{0};$chamber=Get-Native 'player.GetCurrentAmmoRounds'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') ($inventory-$worldRounds) 'Eject debits only magazine rounds'
Write-Output "World=$worldRounds chamber=$chamber ref=$partId"
if($SaveReload){
 Send-Game @('save SC_PhysicalParts_QA','load SC_PhysicalParts_QA')|Out-Null;Start-Sleep -Seconds 5
 Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0','hand l grip pose -0.5 0.75 -0.3 0 30 0');Start-Sleep -Seconds 1
 $a=Get-Addon
 Assert-Equal (@($a.physicalParts|Where-Object ref -eq $partId).Count -eq 1) $true 'World magazine survives normal save/load before recovery'
 Send-Game @('sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
 Send-Game @(('sc_pick {0:X8}' -f $Weapon))|Out-Null;Start-Sleep -Seconds 1
 Send-XR @('hand r grip pose 0.55 0.95 -0.45 0 55 0');Start-Sleep -Milliseconds 500
 if((Get-Addon).pouch.installedMode -ne 4){Send-XR @('btn a press 180');Start-Sleep -Seconds 2}
 $a=Get-Addon
 Assert-Equal $a.insertion.valid $true 'Native weapon redrawn after normal load'
 # TTR's native draw may fill a new magazine; account for that second world part.
 $inventory=(Get-Native 'player.GetItemCount (player.GetCurrentAmmo)')+$worldRounds
 $chamber=Get-Native 'player.GetCurrentAmmoRounds'
}
function Approach-PartForFilm([uint32]$PartId){
 if(!$Tutorial){return}
 for($walk=0;$walk -lt 3;$walk++){
  $a=Get-Addon;$part=@($a.physicalParts|Where-Object ref -eq $PartId)[0]
  if(!$part){throw 'The dropped part is no longer loaded'}
  $dx=$part.tracking[0]-$a.nativeHeadTracking[0];$dy=$part.tracking[1]-$a.nativeHeadTracking[1];$dz=$part.tracking[2]-$a.nativeHeadTracking[2]
  $range=[Math]::Sqrt($dx*$dx+$dz*$dz)
  $yaw=[Math]::Atan2(-$dx,-$dz)*180/[Math]::PI;$pitch=[Math]::Atan2($dy,$range)*180/[Math]::PI
  Send-XR @(("head to 0 1.1 0 {0:F3} {1:F3} 0 1000" -f $yaw,$pitch));Start-Sleep -Milliseconds 1200
  if($range -lt 1.8){return}
  Mark 'Move closer to the dropped magazine'
  Send-XR @('stick l 0 .65');Start-Sleep -Milliseconds 800
  Send-XR @('stick l center');Start-Sleep -Milliseconds 500
  $a=Get-Addon;$part=@($a.physicalParts|Where-Object ref -eq $PartId)[0]
  $nx=$part.tracking[0]-$a.nativeHeadTracking[0];$nz=$part.tracking[2]-$a.nativeHeadTracking[2]
  if([Math]::Sqrt($nx*$nx+$nz*$nz) -ge $range-.05){throw 'Native walking did not approach the magazine; stop the take'}
 }
 throw 'Magazine remains beyond the bounded recovery approach'
}
$well=@($a.insertion.target)
Send-XR @('head to 0 1.1 0 -25 -65 0 1600');Start-Sleep -Milliseconds 1800
Mark 'Dropped part remains in the world';Start-Sleep -Seconds 2
Approach-PartForFilm $partId
Mark 'Original gravity-glove pickup'
if($Gravity){Pull-PhysicalPart $partId}else{Grab-PhysicalPart $partId}
Assert-Equal (Get-Addon).nativeGrabRef ([uint32]$partId) 'Normal world grip picks up recovered magazine'
Mark 'Recovered in normal pickup mode'
Send-XR @('head to 0 1.1 0 0 -25 0 1600','hand r to grip 0.25 0.95 -0.48 0 55 0 1400','hand l to grip -0.28 0.9 -0.4 -4 55 -6 1400');Start-Sleep -Milliseconds 1800
Start-Sleep -Seconds 2
Mark 'Bring magazine to the gun'

for($i=0;$i -lt 6;$i++){
 $a=Get-Addon;$t=$a.insertion.target;$p=@($a.physicalParts|Where-Object ref -eq $partId)[0];$c=$p.tracking
 if($a.physicalMags.recovered -gt $recoverBefore){break}
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $px=$s.handL.pos[0]+$t[0]-$c[0];$py=$s.handL.pos[1]+$t[1]-$c[1];$pz=$s.handL.pos[2]+$t[2]-$c[2]
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 900" -f $px,$py,$pz));Start-Sleep -Milliseconds 1100
}
$a=Get-Addon;$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-reinsert-handoff-$Weapon-$Rounds.json"
Assert-Equal ($a.physicalMags.recovered -gt $recoverBefore) $true 'Recovered part enters original full native magazine initializer'
Assert-Equal $a.physicalMags.recoveredRounds $worldRounds 'Recovered native magazine keeps exact partial or empty count'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Recovery restores exactly the world rounds once'
Mark 'Native manual reload takes over'
# Native insertion requires leaving its corridor once, then approaching while gripping.
Send-XR @('hand l to grip -0.30 0.75 -0.35 -4 55 -6 1200');Start-Sleep -Milliseconds 1400
for($align=0;$align -lt 3;$align++){
 $a=Get-Addon;$angles=(& python.exe "$PSScriptRoot\magazine-pose.py" align)|ConvertFrom-Json
 $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 800" -f $state.handL.pos[0],$state.handL.pos[1],$state.handL.pos[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 950
}
Mark 'Align and insert'
for($i=0;$i -lt 10;$i++){
 $a=Get-Addon;if($a.pouch.installedMode -eq 0){break}
 $t=$a.insertion.target;$c=$a.insertion.held
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $px=$s.handL.pos[0]+$t[0]-$c[0];$py=$s.handL.pos[1]+$t[1]-$c[1];$pz=$s.handL.pos[2]+$t[2]-$c[2]
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 1400" -f $px,$py,$pz,$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 1600
 $a.insertion|ConvertTo-Json -Compress|Write-Output
}
Send-XR @('grip l 0');Start-Sleep -Milliseconds 400
$a=Get-Addon;$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-reinsert-state-$Weapon-$Rounds.json"
Save-Eyes "magazine-reinsert-result-$Weapon-$Rounds"
Assert-Equal $a.pouch.installedMode 0 'Recovered magazine inserted through native manual reload'
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') ($worldRounds+$chamber) 'Native loaded count equals recovered magazine plus existing chamber'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Insertion does not duplicate inventory ammunition'

Mark 'Magazine seated by native reload'
Send-XR @('hand l to grip -0.28 0.8 -0.25 -5 40 -5 1200');Start-Sleep -Milliseconds 1500
Start-Sleep -Seconds 2
$loadedBefore=Get-Native 'player.GetCurrentAmmoRounds'
Send-XR @('trigger r pull 100');Start-Sleep -Milliseconds 800
$loadedAfter=Get-Native 'player.GetCurrentAmmoRounds'
Assert-Equal ($loadedAfter -lt $loadedBefore) $true 'Native firing uses the reinserted magazine'
$inventory-=($loadedBefore-$loadedAfter)
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Native firing debits its actual ammo use'
Mark 'Fire normally';Start-Sleep -Seconds 2
# A second native eject/recover cycle returns the same rounds to the original pouch.
Send-XR @('hand l to grip -0.5 0.75 -0.3 0 30 0 650');Start-Sleep -Milliseconds 800
Send-XR @('btn a press 180');Start-Sleep -Seconds 2
$a=Get-Addon;$partId=$a.physicalMags.lastRef;$pouchBefore=$a.physicalMags.recoveredPouch
Mark 'Recover for the ammo pouch'
Send-XR @('head to 0 1.1 0 -25 -65 0 1400');Start-Sleep -Milliseconds 1600
Approach-PartForFilm $partId
Pull-PhysicalPart $partId
Mark 'Return the recovered part to the pouch'
Send-XR @('head to 0 1.1 0 0 -62 0 1600');Start-Sleep -Milliseconds 1700
Send-XR @('hand r to grip 0.45 1.05 -0.25 -30 55 5 700');Start-Sleep -Milliseconds 850
for($i=0;$i -lt 5;$i++){
 $a=Get-Addon;$part=@($a.physicalParts|Where-Object ref -eq $partId)[0];$t=$a.physicalPouchTracking;$c=$part.tracking
 $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $px=$state.handL.pos[0]+$t[0]-$c[0];$py=$state.handL.pos[1]+$t[1]-$c[1];$pz=$state.handL.pos[2]+$t[2]-$c[2]
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 850" -f $px,$py,$pz));Start-Sleep -Milliseconds 1000
}
Send-XR @('grip l 0');Start-Sleep -Milliseconds 700
$a=Get-Addon;$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-reinsert-pouch-$Weapon-$Rounds.json"
Assert-Equal ($a.physicalMags.recoveredPouch -gt $pouchBefore) $true 'Normal grip release returns recovered magazine to native ammo pouch'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Repeated eject and pouch return preserve exact ammunition'

Mark 'Release grip to store';Start-Sleep -Seconds 2
Send-XR @('head to 0 1.1 0 0 -25 0 1400','hand l to grip -0.25 0.8 -0.25 -5 40 -5 1400');Start-Sleep -Milliseconds 1600
Finish-Take
} finally {Stop-Take}
