param([uint32]$WeaponId=0x0000434F,[string]$Label="pistol")
. "$PSScriptRoot\casual-guide-helpers.ps1"
. "$PSScriptRoot\magazine-helpers.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\physical-parts'
New-Item -ItemType Directory -Force $script:Guide|Out-Null
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -20 0','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 500}
Park-Hands
Send-Game @('coc GSDocMitchellHouse')|Out-Null
Start-Sleep -Seconds 4
# Clear the chair occupying the actor's standing capsule in this unsaved capture scene.
Send-Game @('00103E44.Disable')|Out-Null
Send-Game @('player.SetPos X 1245','player.SetPos Y 905','player.SetPos Z 7360','player.SetAngle Z 0')|Out-Null
Send-XR @('head pose 0 1.1 0 0 -25 0','hand l grip pose -0.23 0.75 -0.2 -8 55 -8','hand r grip pose 0.22 0.95 -0.42 4 55 5')
Start-Sleep -Seconds 1
Send-Game @(('player.AddItem {0:X8} 1' -f $WeaponId))|Out-Null
Send-Game @('sc_weapons')|Out-Null
Start-Sleep -Milliseconds 350
$a=Get-Addon
$dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1];$len=[Math]::Sqrt($dx*$dx+$dy*$dy);$rx=$dx/$len;$ry=$dy/$len
Send-Game @(('sc_pick {0:X8}' -f $WeaponId))|Out-Null
Start-Sleep -Seconds 1
$ammo=Get-Native 'GetWeaponAmmo (player.GetEquippedObject 5)'
if((Get-Native 'GetType (GetWeaponAmmo (player.GetEquippedObject 5))') -eq 85){$ammo=Get-Native 'ListGetNthForm (GetWeaponAmmo (player.GetEquippedObject 5)) 0'}
Send-Game @(('player.AddItem {0:X8} 200' -f $ammo))|Out-Null
$capacity=Get-Native 'GetWeaponClipRounds (player.GetEquippedObject 5)'
if($WeaponId -eq 0x434F){
 if((Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null}
 Send-Game @('player.SetCurrentAmmoRounds 1')|Out-Null
 $attempts=(Get-Addon).physicalMags.attempts
 Send-XR @('trigger r pull 180');Start-Sleep -Seconds 2
 Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') $capacity 'Automatic reload still follows native game behavior'
 Assert-Equal (Get-Addon).physicalMags.attempts $attempts 'Automatic reload creates no addon physical transaction'
}else{Send-Game @("player.SetCurrentAmmoRounds $capacity")|Out-Null}
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null}
Start-Sleep -Seconds 1
Save-Eyes "magazine-film-$Label-table-setup"
$script:events=[Collections.Generic.List[object]]::new()
Start-Take "$Label-table" 75
try {
 Mark 'Manual reload: original pouch'
 Send-XR @('head to 0 1.1 0 -3 -38 0 1200');Start-Sleep -Milliseconds 1500
 $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$target=$a.pouch.center
 $ex=$target[0]-$a.palms[0][0];$ey=$target[1]-$a.palms[0][1];$ez=$target[2]-$a.palms[0][2]
 $px=$s.handL.pos[0]+($rx*$ex+$ry*$ey)/70;$py=$s.handL.pos[1]+$ez/70;$pz=$s.handL.pos[2]-(-$ry*$ex+$rx*$ey)/70
 Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} -5 55 -5 1500 -0.02 0.04 0" -f $px,$py,$pz));Start-Sleep -Milliseconds 1700
 Send-XR @('grip l 1');Start-Sleep -Milliseconds 400
 Assert-Equal (Get-Addon).pouch.mode 1 'Filmed normal native pouch grab'
 if($Label -eq 'carbine'){
  $check=Get-Addon;$countBefore=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
  Send-XR @('grip l 0');Start-Sleep -Milliseconds 350
  $returned=Get-Addon
  Assert-Equal $returned.physicalMags.successes $check.physicalMags.successes 'Returning a spare to the pouch creates no world part'
  Assert-Equal ($returned.physicalMags.pouchReturns -gt $check.physicalMags.pouchReturns) $true 'Native pouch return path retained'
  Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $countBefore 'Pouch return preserves ammunition'
  Send-XR @('grip l 1');Start-Sleep -Milliseconds 400
  Assert-Equal (Get-Addon).pouch.mode 1 'Native pouch supplies the next magazine normally'
 }
 Send-XR @('hand l curve grip -0.2 0.95 -0.5 -4 55 -8 1400 -0.025 0.03 0','head to 0 1.1 0 0 -20 0 1400');Start-Sleep -Milliseconds 1700
 Mark 'Loaded magazine in hand';Start-Sleep -Seconds 2
 $inventory=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)';$rounds=(Get-Addon).pouch.heldRounds
 Send-XR @('hand r curve grip 0.45 1.0 -0.25 -45 55 5 1300 0.01 0.02 0');Start-Sleep -Milliseconds 1450
 $dropZ=if($Label -eq 'pistol'){'-0.80'}else{'-0.60'}
 Send-XR @("hand l curve grip -0.10 0.95 $dropZ -4 55 -8 1300 -0.015 0.03 0");Start-Sleep -Milliseconds 1500
 Mark 'Drop onto the table';Send-XR @('grip l 0','head to 0 1.1 0 -4 -48 0 1400','hand l curve grip -0.30 0.75 -0.25 -8 55 -8 1300 -0.02 0.03 0');Start-Sleep -Seconds 3
 $a=Get-Addon;$partId=$a.physicalMags.lastRef;$part=@($a.physicalParts|Where-Object ref -eq $partId)[0]
 Assert-Equal ($partId -gt 0 -and $part.center[2] -gt 7414 -and $part.center[2] -lt 7425) $true 'Filmed magazine rests on the native tabletop'
 Save-Eyes "magazine-film-$Label-table-dropped"
 Mark 'Normal grip pickup'
 Send-XR @('head to 0 1.1 0 -4 -45 0 1600','hand r curve grip 0.45 1.0 -0.25 -45 55 5 1500 0.01 0.02 0');Start-Sleep -Milliseconds 1800
 $a=Get-Addon;$part=@($a.physicalParts|Where-Object ref -eq $partId)[0];$t=$part.tracking
 $px=$t[0];$py=$t[1]-$a.heightOffset+.065;$pz=$t[2]
 Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} -4 55 -6 1600 -0.02 0.12 0.03" -f $px,($py+.10),$pz));Start-Sleep -Milliseconds 1700
 $a=Get-Addon;$part=@($a.physicalParts|Where-Object ref -eq $partId)[0];$t=$part.tracking
 $px=$t[0];$py=$t[1]-$a.heightOffset+.065;$pz=$t[2]
 Send-XR @(("hand l curve grip {0:F5} {1:F5} {2:F5} -4 55 -6 450 0 0.01 0" -f $px,$py,$pz));Start-Sleep -Milliseconds 500
 # Grip near the actual surface; driving an open hand into the center knocks small parts away.
 $a=Get-Addon;$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-$Label-pre-contact.json"
 Grab-PhysicalPart $partId 450
 Assert-Equal (Get-Addon).nativeGrabRef ([uint32]$partId) 'Filmed normal grip picks up the physical magazine'
 Send-XR @('head to 0 1.1 0 0 -20 0 1800','hand l curve grip -0.2 0.95 -0.45 -4 55 -8 1800 -0.02 0.04 0','hand r curve grip 0.22 0.95 -0.42 4 55 5 1800 0.01 0.03 0');Start-Sleep -Milliseconds 2000
 Mark 'Recovered and throwable';Start-Sleep -Seconds 3
 Save-Eyes "magazine-film-$Label-held"
 Send-XR @('hand l curve grip -0.24 0.90 -0.32 -4 55 -8 850 -0.02 0.02 0');Start-Sleep -Milliseconds 950
 Mark 'Native throw'
 Send-XR @('hand l curve grip -0.17 0.96 -0.65 -2 55 -6 500 0.01 0.02 0');Start-Sleep -Milliseconds 310
 Send-XR @('grip l 0');Start-Sleep -Milliseconds 500
 Assert-Equal (Get-Addon).nativeGrabRef 0 'Filmed native grip release throws the object'
 Send-XR @('head to 0 1.1 0 -3 -45 0 1300','hand l curve grip -0.26 0.75 -0.25 -8 55 -8 1400 -0.02 0.03 0');Start-Sleep -Seconds 3
 Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') ($inventory-$rounds) 'Filmed toss preserves the ammo in its world reference'
 Mark 'Still a real world item'
 Save-Eyes "magazine-film-$Label-thrown"
 Finish-Take
} finally {Stop-Take}

# Native X Take, after the filmed take. No scripted activation is used here.
$a=Get-Addon;$part=@($a.physicalParts|Where-Object ref -eq $partId)[0]
Grab-PhysicalPart $partId
Assert-Equal (Get-Addon).nativeGrabRef ([uint32]$partId) 'Regrab before native X Take'
Send-XR @('btn x press 180');Start-Sleep -Milliseconds 600
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Native X Take restores exactly the dropped rounds'
