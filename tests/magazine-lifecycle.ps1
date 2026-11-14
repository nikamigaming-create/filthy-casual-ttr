param([uint32]$WeaponId=0x0000434F,[string]$Label="pistol")
. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @(('player.AddItem {0:X8} 1' -f $WeaponId))|Out-Null
Send-Game @('sc_weapons') | Out-Null
Start-Sleep -Milliseconds 400
$a=Get-Addon
$dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1]
$len=[Math]::Sqrt($dx*$dx+$dy*$dy);$rx=$dx/$len;$ry=$dy/$len
$gun=@($a.previews | Where-Object {$_.slot -ne 6 -and $_.form -eq $WeaponId})
Send-Game @(('sc_pick {0:X8}' -f $WeaponId)) | Out-Null
Start-Sleep -Milliseconds 600
Assert-Equal (Get-Native 'player.GetEquippedObject 5') $WeaponId "Fixture selected $Label"
$ammo=Get-Native 'GetWeaponAmmo (player.GetEquippedObject 5)'
$type=Get-Native 'GetType (GetWeaponAmmo (player.GetEquippedObject 5))'
if($type -eq 85){$ammo=Get-Native 'ListGetNthForm (GetWeaponAmmo (player.GetEquippedObject 5)) 0'}
Assert-Equal ($ammo -gt 0) $true "Fixture ammo resolves for $Label"
Send-Game @(('player.AddItem {0:X8} 200' -f $ammo))|Out-Null
$capacity=Get-Native 'GetWeaponClipRounds (player.GetEquippedObject 5)'
Send-Game @("player.SetCurrentAmmoRounds $capacity")|Out-Null
$inventoryBefore=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
$before=Get-Addon
if(!$before.manualReload){Send-Game @('sc_reload') | Out-Null}
Send-XR @('hand r grip pose 0.30 0.95 -0.45 0 55 0','hand l grip pose -0.30 0.65 -0.1 0 30 0')
Start-Sleep -Seconds 1
$records=[Collections.Generic.List[object]]::new()
function Record([string]$name){$v=Get-Addon;$records.Add([ordered]@{event=$name;pouch=$v.pouch;trace=$v.reloadTrace;physical=$v.physicalMags;ammo=(Get-Native 'player.GetCurrentAmmoRounds');inventory=(Get-Native 'player.GetItemCount (player.GetCurrentAmmo)')});$v.pouch|ConvertTo-Json -Compress -Depth 6|Write-Output}
Record 'manual-start'
for($i=0;$i -lt 4;$i++){
 $a=Get-Addon;$target=$a.pouch.center
 if($target.Count -ne 3){throw 'Missing native pouch center'}
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $ex=$target[0]-$a.palms[0][0];$ey=$target[1]-$a.palms[0][1];$ez=$target[2]-$a.palms[0][2]
 $px=$s.handL.pos[0]+($rx*$ex+$ry*$ey)/70
 $py=$s.handL.pos[1]+$ez/70
 $pz=$s.handL.pos[2]-(-$ry*$ex+$rx*$ey)/70
 Send-XR @(("hand l grip pos {0:F5} {1:F5} {2:F5}" -f $px,$py,$pz));Start-Sleep -Milliseconds 200
}
Send-XR @('grip l 1');Start-Sleep -Milliseconds 350
Send-XR @('hand l to grip -0.6 1.2 -0.7 0 30 0 600');Start-Sleep -Milliseconds 750
$held=Get-Addon
$rounds=$held.pouch.heldRounds
Assert-Equal ($held.pouch.mode -eq 1 -and $rounds -gt 0) $true "Native pouch supplies $Label reload part"
Record 'held'
Save-Eyes "magazine-$Label-held"
Send-XR @('grip l 0');Start-Sleep -Milliseconds 100
Record 'released'
Start-Sleep -Milliseconds 250
Record 'falling'
Start-Sleep -Seconds 2
Record 'after-timeout'
$a=Get-Addon
if($a.physicalMags.active){
 Assert-Equal ($a.physicalMags.lastRef -gt 0) $true 'Release creates a real world reference'
 Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') ($inventoryBefore-$rounds) "Exactly one $rounds-round stack left inventory"
 $id=('{0:X8}' -f [uint32]$a.physicalMags.lastRef)
 Send-XR @('head to 0 1.1 0 -45 -50 0 700');Start-Sleep -Milliseconds 900
 Save-Eyes "magazine-$Label-world"
 # Lean down into normal physical reach; do not demand an impossibly long arm.
 Send-XR @('head to 0 1.1 0 0 -45 0 700');Start-Sleep -Milliseconds 1000
 # Approach just above the collision boundary, then squeeze. An open hand
 # driven through a tiny cell knocks it away before the grip action.
 for($reach=0;$reach -lt 5;$reach++){
  $a=Get-Addon
  if($a.nativeGrabRef -eq $a.physicalMags.lastRef){break}
  $part=@($a.physicalParts|Where-Object ref -eq $a.physicalMags.lastRef)[0]
  if(!$part){throw 'Physical magazine model is not attached'}
  Send-XR @('grip l 0')
  $target=$part.tracking
  $px=$target[0];$py=$target[1]-$a.heightOffset+0.03;$pz=$target[2]
  Send-XR @(("hand l grip pos {0:F5} {1:F5} {2:F5}" -f $px,$py,$pz));Start-Sleep -Milliseconds 120
  Send-XR @('grip l 1');Start-Sleep -Milliseconds 250
 }
 $a=Get-Addon
 $a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-$Label-before-grip.json"
 Assert-Equal $a.nativeGrabRef ([uint32]$a.physicalMags.lastRef) 'Normal native grip picks up the dropped magazine'
 Send-XR @('hand l to grip -0.25 1.0 -0.4 0 30 0 700');Start-Sleep -Milliseconds 800
 Save-Eyes "magazine-$Label-recovered"
 Send-XR @('hand l to grip -0.2 1.2 -1.0 0 30 0 240');Start-Sleep -Milliseconds 180
 Send-XR @('grip l 0');Start-Sleep -Milliseconds 100
 Assert-Equal (Get-Addon).nativeGrabRef 0 'Native release throws the magazine'
 Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') ($inventoryBefore-$rounds) 'Pickup and throw preserve the magazine ammo outside inventory'
 Start-Sleep -Milliseconds 750
 Save-Eyes "magazine-$Label-thrown"
 Send-Game @("$id.Activate player") | Out-Null
 Start-Sleep -Milliseconds 400
 Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventoryBefore 'Native recovery returns exactly the dropped rounds'
}
Save-Eyes "magazine-$Label-recovered-ammo"
$records | ConvertTo-Json -Depth 10 | Set-Content "$script:Repo\evidence\magazine-$Label-lifecycle.json"
if(!$before.manualReload){Send-Game @('sc_reload') | Out-Null}
