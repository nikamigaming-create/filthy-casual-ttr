param([switch]$Ground,[switch]$Insert)
. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -20 0','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 5
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
$weapon=Get-Native 'WeapNVGrenadeRifle'
Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 200
Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Seconds 1
$ammo=Get-Native 'GetWeaponAmmo (player.GetEquippedObject 5)'
if((Get-Native 'GetType (GetWeaponAmmo (player.GetEquippedObject 5))') -eq 85){$ammo=Get-Native 'ListGetNthForm (GetWeaponAmmo (player.GetEquippedObject 5)) 0'}
Send-Game @(('player.AddItem {0:X8} 50' -f [uint32]$ammo))|Out-Null
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null}
Send-XR @('hand r grip pose .3 .95 -.45 0 55 0','hand l grip pose -.3 .7 -.1 0 30 0');Start-Sleep -Milliseconds 600
if($Insert){Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 800}
Send-XR @('btn a down','hand r to grip .3 .95 -.45 0 -10 0 180');Start-Sleep -Milliseconds 450
Send-XR @('btn a up','hand r to grip .3 .95 -.45 0 55 0 400');Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).pouch.shown $true 'Native ammo pouch is visible before reaching'
$inventory=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
function Move-Left([double[]]$Target,[double[]]$Current,[int]$Duration=280){
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $p=@(0,0,0);for($i=0;$i -lt 3;$i++){$p[$i]=$s.handL.pos[$i]+$Target[$i]-$Current[$i]}
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 {3}" -f $p[0],$p[1],$p[2],$Duration));Start-Sleep -Milliseconds ($Duration+150)
}
for($i=0;$i -lt 3;$i++){$a=Get-Addon;Move-Left $a.physicalPouchTracking $a.nativeGripTracking[0]}
Send-XR @('grip l 1');Start-Sleep -Milliseconds 400
Send-XR @('hand l to grip -.5 .95 -.5 -4 55 -6 450');Start-Sleep -Milliseconds 600
$a=Get-Addon;Assert-Equal $a.pouch.mode 1 'Grenade round held through native pouch'
$rounds=$a.pouch.heldRounds
Send-XR @('grip l 0');Start-Sleep -Seconds 2
$a=Get-Addon;$id=[uint32]$a.physicalMags.lastRef;$before=$a.physicalMags.recovered
Assert-Equal ($id -gt 0) $true 'Dropped grenade round exists in world'
. "$PSScriptRoot\magazine-gravity.ps1"
if($Ground){
 Send-XR @('head to .15 .35 -.15 0 -40 0 650');Start-Sleep -Milliseconds 800
 Grab-PhysicalPart $id -NativeReload
}else{Pull-PhysicalPart $id -NativeReload}
Start-Sleep -Milliseconds 600
$a=Get-Addon;$a|ConvertTo-Json -Depth 16|Set-Content "$script:Repo\work\reload-grip\grenade-recovered.json"
Assert-Equal ($a.physicalMags.recovered -gt $before) $true 'Normal pickup immediately enters native insertion-ready grip'
Assert-Equal $a.pouch.mode 1 'Recovered grenade uses native held-round mode'
Assert-Equal $a.pouch.heldRounds $rounds 'Recovered grenade preserves exact count'
Assert-Equal $a.nativeGrabRef 0 'Generic physics grab released to native reload hand'
Send-XR @('hand l to grip -.23 .9 -.43 -4 55 -6 500');Start-Sleep -Milliseconds 650
Save-Eyes 'recovery112-grenade-held'
for($i=0;$i -lt 3;$i++){$a=Get-Addon;Move-Left $a.physicalPouchTracking $a.pouch.heldBoundTracking}
Send-XR @('grip l 0');Start-Sleep -Milliseconds 500
$a=Get-Addon;$a|ConvertTo-Json -Depth 16|Set-Content "$script:Repo\work\reload-grip\grenade-stowed.json"
Assert-Equal $a.pouch.mode 0 'Recovered grenade released into native pouch'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Pouch return preserves ammunition exactly'
if($Insert){
 for($n=0;$n -lt 3;$n++){Send-XR @('btn menu press 80');Start-Sleep -Milliseconds 160}
 Start-Sleep -Milliseconds 700
 # Fetch the returned round using native pouch controls and insert it.
 Send-XR @('grip l 1');Start-Sleep -Milliseconds 400
 Send-XR @('hand l to grip -.23 .9 -.43 -4 55 -6 500');Start-Sleep -Milliseconds 650
 $target=@((Get-Addon).tutorial.namedParts|Where-Object node -eq '##Shells')[0]
 $target|Add-Member -NotePropertyName heldOccurrence -NotePropertyValue 1
 $target|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\work\tutorial-dev\loader-target.json"
 for($i=0;$i -lt 3;$i++){
  $null=Get-Addon;$pose=(& python.exe "$PSScriptRoot\tutorial-loader-pose.py")|ConvertFrom-Json;$angles=$pose.angles
  $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 400" -f $state.handL.pos[0],$state.handL.pos[1],$state.handL.pos[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 550
 }
 for($i=0;$i -lt 4;$i++){
  $a=Get-Addon;if((Get-Native 'player.GetCurrentAmmoRounds') -gt 0){break}
  $pose=(& python.exe "$PSScriptRoot\tutorial-loader-pose.py")|ConvertFrom-Json
  $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $port=@($a.tutorial.parts|Where-Object node -eq 'loading-port')[0].tracking
  $p=@(0,0,0);for($j=0;$j -lt 3;$j++){$p[$j]=$state.handL.pos[$j]+$port[$j]-$pose.held[$j]}
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 550" -f $p[0],$p[1],$p[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 700
 }
 Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 1 'Returned grenade inserts through native loading port'
 Send-XR @('grip l 0','hand l to grip -.3 .7 -.15 0 30 0 400','btn a down','hand r to grip .3 .95 -.45 0 115 0 180');Start-Sleep -Milliseconds 400
 Send-XR @('btn a up','hand r to grip .3 .95 -.45 0 55 0 400');Start-Sleep -Milliseconds 600
 Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 700
 Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 0 'Native reloaded grenade fires'
 Save-Eyes 'recovery112-grenade-fired'
}
Write-Output 'PASS recovered grenade native grip and pouch return' 
