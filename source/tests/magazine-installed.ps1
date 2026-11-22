param([uint32[]]$Weapons=@(0x0000434F,0x0008F21E))
. "$PSScriptRoot\magazine-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -20 0','btn a press 180')
foreach($weapon in $Weapons){
 Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 4
 if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
 Park-Hands
 Send-Game @(('player.AddItem {0:X8} 1' -f $weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
 Send-Game @(('sc_pick {0:X8}' -f $weapon))|Out-Null;Start-Sleep -Seconds 1
 if((Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 400}
 $ammo=Get-Native 'GetWeaponAmmo (player.GetEquippedObject 5)'
 if((Get-Native 'GetType (GetWeaponAmmo (player.GetEquippedObject 5))') -eq 85){$ammo=Get-Native 'ListGetNthForm (GetWeaponAmmo (player.GetEquippedObject 5)) 0'}
 Send-Game @(('player.AddItem {0:X8} 200' -f $ammo))|Out-Null
 $capacity=Get-Native 'GetWeaponClipRounds (player.GetEquippedObject 5)'
 Send-Game @("player.SetCurrentAmmoRounds $capacity")|Out-Null
 Start-Sleep -Milliseconds 500
 $inventory=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
 if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null}
 Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0','hand l grip pose -0.3 0.8 -0.3 0 30 0');Start-Sleep -Seconds 1
 $a=Get-Addon;$a.installedContact|ConvertTo-Json -Compress|Write-Output
 if($a.installedContact.looseRounds -or $a.installedContact.blocked){Write-Output "Native extraction ineligible for $weapon";continue}
 $before=$a.physicalMags.successes
 Send-XR @('grip l 1');Start-Sleep -Milliseconds 200
 for($i=0;$i -lt 5;$i++){
  $a=Get-Addon;$t=$a.installedPartTracking;$c=$a.installedContact.tracking
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $px=$s.handL.pos[0]+$t[0]-$c[0];$py=$s.handL.pos[1]+$t[1]-$c[1];$pz=$s.handL.pos[2]+$t[2]-$c[2]
  Send-XR @(("hand l grip pos {0:F5} {1:F5} {2:F5}" -f $px,$py,$pz));Start-Sleep -Milliseconds 250
 }
 $a=Get-Addon;$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-installed-$weapon-contact.json"
 Send-XR @('btn a press 180');Start-Sleep -Milliseconds 350
 Send-XR @('hand l to grip -0.4 0.8 -0.55 0 30 0 700');Start-Sleep -Milliseconds 900
  $a=Get-Addon;$a.pouch|ConvertTo-Json -Compress|Write-Output
 Assert-Equal ($a.pouch.installedMode -in @(1,2)) $true 'Native Save Ammo enters extraction state'
 $rounds=$a.pouch.installedRounds
 Send-XR @('grip l 0');Start-Sleep -Seconds 2
 $a=Get-Addon;$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-installed-$weapon-result.json"
  Assert-Equal ($a.physicalMags.successes -gt $before) $true "Native installed-magazine release for $weapon"
 $part=@($a.physicalParts|Where-Object ref -eq $a.physicalMags.lastRef)[0]
 Assert-Equal $part.stackCount $rounds 'Hand-pulled magazine preserves its native round count'
 Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') ($inventory-$rounds) 'Hand-pulled magazine debits ammunition exactly once'
}