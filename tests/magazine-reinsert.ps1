param([uint32]$Weapon=0x434F,[int]$Rounds=6,[switch]$SaveReload,[switch]$Gravity)
. "$PSScriptRoot\magazine-helpers.ps1"
. "$PSScriptRoot\magazine-gravity.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -25 0','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
$recoverBefore=(Get-Addon).physicalMags.recovered
Park-Hands
Send-Game @(('player.AddItem {0:X8} 1' -f $Weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @(('sc_pick {0:X8}' -f $Weapon))|Out-Null;Start-Sleep -Seconds 1
if((Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 300}
$ammo=Get-Native 'GetWeaponAmmo (player.GetEquippedObject 5)'
if((Get-Native 'GetType (GetWeaponAmmo (player.GetEquippedObject 5))') -eq 85){$ammo=Get-Native 'ListGetNthForm (GetWeaponAmmo (player.GetEquippedObject 5)) 0'}
Send-Game @(('player.AddItem {0:X8} 100' -f $ammo),"player.SetCurrentAmmoRounds $Rounds")|Out-Null;Start-Sleep -Milliseconds 600
$inventory=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
Send-Game @('sc_reload')|Out-Null
Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0','hand l grip pose -0.5 0.75 -0.3 0 30 0');Start-Sleep -Seconds 1
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
$well=@($a.insertion.target)
if($Gravity){Pull-PhysicalPart $partId}else{Grab-PhysicalPart $partId}
Assert-Equal (Get-Addon).nativeGrabRef ([uint32]$partId) 'Normal world grip picks up recovered magazine'
Send-XR @('hand l to grip -0.28 0.9 -0.4 -4 55 -6 600');Start-Sleep -Milliseconds 800
for($i=0;$i -lt 6;$i++){
 $a=Get-Addon;$t=$a.insertion.target;$p=@($a.physicalParts|Where-Object ref -eq $partId)[0];$c=$p.tracking
 if($a.physicalMags.recovered -gt $recoverBefore){break}
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $px=$s.handL.pos[0]+$t[0]-$c[0];$py=$s.handL.pos[1]+$t[1]-$c[1];$pz=$s.handL.pos[2]+$t[2]-$c[2]
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 400" -f $px,$py,$pz));Start-Sleep -Milliseconds 550
}
$a=Get-Addon;$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-reinsert-handoff-$Weapon-$Rounds.json"
Assert-Equal ($a.physicalMags.recovered -gt $recoverBefore) $true 'Recovered part enters original full native magazine initializer'
Assert-Equal $a.physicalMags.recoveredRounds $worldRounds 'Recovered native magazine keeps exact partial or empty count'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Recovery restores exactly the world rounds once'
# Native insertion requires leaving its corridor once, then approaching while gripping.
Send-XR @('hand l to grip -0.30 0.75 -0.35 -4 55 -6 700');Start-Sleep -Milliseconds 850
for($align=0;$align -lt 3;$align++){
 $a=Get-Addon;$angles=(& python.exe "$PSScriptRoot\magazine-pose.py" align)|ConvertFrom-Json
 $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 550" -f $state.handL.pos[0],$state.handL.pos[1],$state.handL.pos[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 700
}
for($i=0;$i -lt 10;$i++){
 $a=Get-Addon;if($a.pouch.installedMode -eq 0){break}
 $t=$a.insertion.target;$c=$a.insertion.held
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $px=$s.handL.pos[0]+$t[0]-$c[0];$py=$s.handL.pos[1]+$t[1]-$c[1];$pz=$s.handL.pos[2]+$t[2]-$c[2]
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 500" -f $px,$py,$pz,$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 650
 $a.insertion|ConvertTo-Json -Compress|Write-Output
}
Send-XR @('grip l 0');Start-Sleep -Milliseconds 400
$a=Get-Addon;$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-reinsert-state-$Weapon-$Rounds.json"
Save-Eyes "magazine-reinsert-result-$Weapon-$Rounds"
Assert-Equal $a.pouch.installedMode 0 'Recovered magazine inserted through native manual reload'
Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') ($worldRounds+$chamber) 'Native loaded count equals recovered magazine plus existing chamber'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Insertion does not duplicate inventory ammunition'

# A second native eject/recover cycle returns the same rounds to the original pouch.
Send-XR @('hand l to grip -0.5 0.75 -0.3 0 30 0 650');Start-Sleep -Milliseconds 800
Send-XR @('btn a press 180');Start-Sleep -Seconds 2
$a=Get-Addon;$partId=$a.physicalMags.lastRef;$pouchBefore=$a.physicalMags.recoveredPouch
Grab-PhysicalPart $partId
Send-XR @('hand r to grip 0.45 1.05 -0.25 -30 55 5 700');Start-Sleep -Milliseconds 850
for($i=0;$i -lt 5;$i++){
 $a=Get-Addon;$part=@($a.physicalParts|Where-Object ref -eq $partId)[0];$t=$a.physicalPouchTracking;$c=$part.tracking
 $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $px=$state.handL.pos[0]+$t[0]-$c[0];$py=$state.handL.pos[1]+$t[1]-$c[1];$pz=$state.handL.pos[2]+$t[2]-$c[2]
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 450" -f $px,$py,$pz));Start-Sleep -Milliseconds 600
}
Send-XR @('grip l 0');Start-Sleep -Milliseconds 700
$a=Get-Addon;$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-reinsert-pouch-$Weapon-$Rounds.json"
Assert-Equal ($a.physicalMags.recoveredPouch -gt $pouchBefore) $true 'Normal grip release returns recovered magazine to native ammo pouch'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Repeated eject and pouch return preserve exact ammunition'
