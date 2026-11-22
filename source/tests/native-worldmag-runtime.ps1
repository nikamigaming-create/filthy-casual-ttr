param([uint32]$Weapon=0x434F,[ValidateRange(1,30)][int]$Rounds=6,[ValidateRange(45,300)][int]$MaxSeconds=180,[switch]$Capture,[switch]$EmptyChamber)
# Requires an ALREADY RUNNING bounded isolated QA session, never launches a game.
# Setup uses native inventory/equip commands; acquisition/insertion/return use XR controls only.
. "$PSScriptRoot\magazine-helpers.ps1"
$script:Deadline=[DateTime]::UtcNow.AddSeconds($MaxSeconds)
$script:Progress='preflight';$script:LastAddon=$null;$script:TargetRef=0
$script:ResultPath=Join-Path $script:Repo 'evidence\native-worldmag-runtime.json'
function Check-Budget { if([DateTime]::UtcNow -ge $script:Deadline){throw "Test deadline at $script:Progress"} }
function Read-State {
 Check-Budget
 $script:LastAddon=Get-Addon
 if($script:LastAddon.worldmagRecovery.poisoned){throw "Recovery poisoned: $($script:LastAddon.worldmagRecovery.reason)"}
 return $script:LastAddon
}
function Part($State,[uint32]$Id) { return @($State.nativeWorldMags|Where-Object { $_.ref -eq $Id -and $_.valid })|Select-Object -First 1 }
function Wait-NewPart([object[]]$OldIds) {
 for($i=0;$i -lt 25;$i++) {
  $a=Read-State;$found=@($a.nativeWorldMags|Where-Object { $_.valid -and $_.rootValid -and $_.boundValid -and $_.tracking.Count -eq 3 -and $OldIds -notcontains $_.ref })
  if($found.Count -eq 1){return $found[0]}
  if($found.Count -gt 1){throw 'Ambiguous new native magazine references'}
  Start-Sleep -Milliseconds 120
 }
 throw 'Native eject did not expose exactly one loaded upstream magazine'
}
function Grab-NativeWorldMag([uint32]$Id,[int]$Before) {
 $script:TargetRef=$Id
 Send-XR @('grip l 0');Start-Sleep -Milliseconds 150
 for($i=0;$i -lt 8;$i++) {
  $a=Read-State
  if($a.worldmagRecovery.committed -gt $Before){Assert-Equal $a.worldmagRecovery.transferred $Id 'Bridge consumed the exact normally grabbed upstream reference';return}
  $p=Part $a $Id;if(!$p -or !$p.boundValid -or $p.tracking.Count -ne 3){throw "Target native reference $Id has no valid bound/tracking"}
  if($a.nativeGrabRef -eq $Id){Start-Sleep -Milliseconds 150;continue}
  $sim=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  # Align the ACTUAL native grab point, settle the pose, THEN create a fresh grip edge.
  # Alternate 5mm above bound center and exact center, not the old donor-box +25mm.
  $raise=if(($i%2)-eq 0){.005}else{0.0}
  $x=$sim.handL.pos[0]+$p.tracking[0]-$a.nativeGripTracking[0][0]
  $y=$sim.handL.pos[1]+$p.tracking[1]-$a.nativeGripTracking[0][1]+$raise
  $z=$sim.handL.pos[2]+$p.tracking[2]-$a.nativeGripTracking[0][2]
  Send-XR @('grip l 0');Start-Sleep -Milliseconds 100
  Send-XR @(("hand l grip pose {0:F5} {1:F5} {2:F5} -4 55 -6" -f $x,$y,$z));Start-Sleep -Milliseconds 200
  Send-XR @('grip l 1');Start-Sleep -Milliseconds 250
 }
 throw "Ordinary native grip/bridge did not recover $Id; held=$((Read-State).nativeGrabRef)"
}
function Insert-NativeRecovered {
 $a=Read-State;Assert-Equal $a.insertion.valid $true 'Current-build read-only insertion diagnostics valid'
 Send-XR @('hand l to grip -0.30 0.75 -0.35 -4 55 -6 500');Start-Sleep -Milliseconds 650
 for($i=0;$i -lt 3;$i++) {
  $a=Read-State;Assert-Equal $a.insertion.valid $true 'Insertion geometry remains valid'
  $angles=(& python.exe "$PSScriptRoot\magazine-pose.py" align)|ConvertFrom-Json
  if($LASTEXITCODE -ne 0 -or $angles.Count -ne 3){throw 'Native magazine basis alignment failed'}
  $sim=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 350" -f $sim.handL.pos[0],$sim.handL.pos[1],$sim.handL.pos[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 450
 }
 for($i=0;$i -lt 12;$i++) {
  $a=Read-State;if($a.pouch.installedMode -eq 0){break}
  if(!$a.insertion.valid){throw 'Native well geometry unavailable during insertion'}
  $sim=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $x=$sim.handL.pos[0]+$a.insertion.target[0]-$a.insertion.held[0]
  $y=$sim.handL.pos[1]+$a.insertion.target[1]-$a.insertion.held[1]
  $z=$sim.handL.pos[2]+$a.insertion.target[2]-$a.insertion.held[2]
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 400" -f $x,$y,$z,$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 550
 }
 $a=Read-State;Assert-Equal $a.pouch.installedMode 0 'Recovered magazine inserts through original native manual reload'
 Send-XR @('grip l 0');Start-Sleep -Milliseconds 250
}
function Return-NativePouch {
 for($i=0;$i -lt 6;$i++) {
  $a=Read-State;if(!$a.insertion.valid -or !$a.insertion.pouchValid){throw 'Read-only native held/pouch geometry unavailable'}
  # Native release compares actual grip point34557C/6949C0, not magazine origin.
  $sim=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $x=$sim.handL.pos[0]+$a.insertion.pouchTarget[0]-$a.nativeGripTracking[0][0]
  $y=$sim.handL.pos[1]+$a.insertion.pouchTarget[1]-$a.nativeGripTracking[0][1]
  $z=$sim.handL.pos[2]+$a.insertion.pouchTarget[2]-$a.nativeGripTracking[0][2]
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 350" -f $x,$y,$z));Start-Sleep -Milliseconds 450
 }
 Send-XR @('grip l 0');Start-Sleep -Milliseconds 700
}
try {
 $a=Read-State
 Assert-Equal $a.worldmagRecovery.installed 1 'Exact-build native worldmag recovery is installed'
 Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -25 0','btn a press 180')
 $script:Progress='load QA fixture';Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 5
 if((Read-State).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
 Park-Hands
 $script:Progress='native fixture setup'
 Send-Game @(('player.AddItem {0:X8} 1' -f $Weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
 Send-Game @(('sc_pick {0:X8}' -f $Weapon))|Out-Null;Start-Sleep -Seconds 1
 if((Read-State).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 300}
 $ammo=Get-Native 'GetWeaponAmmo (player.GetEquippedObject 5)'
 if((Get-Native 'GetType (GetWeaponAmmo (player.GetEquippedObject 5))') -eq 85){$ammo=Get-Native 'ListGetNthForm (GetWeaponAmmo (player.GetEquippedObject 5)) 0'}
 Send-Game @(('player.AddItem {0:X8} 100' -f $ammo),"player.SetCurrentAmmoRounds $Rounds")|Out-Null;Start-Sleep -Milliseconds 600
 $inventory=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
 Send-Game @('sc_reload')|Out-Null
 Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0','hand l grip pose -0.5 0.75 -0.3 0 30 0');Start-Sleep -Seconds 1
 $a=Read-State;Assert-Equal $a.manualReload $true 'Native manual reload enabled'
 Assert-Equal $a.insertion.valid $true 'Current-build insertion diagnostic available before ejection'
 if($EmptyChamber){for($j=0;$j -lt 3;$j++){Send-XR @('btn menu press 80');Start-Sleep -Milliseconds 160};Start-Sleep -Milliseconds 600}
 $initialCommitted=$a.worldmagRecovery.committed
 for($cycle=0;$cycle -lt 2;$cycle++) {
  $script:Progress="cycle $cycle native eject"
  Send-XR @('grip l 0','hand l grip pose -0.5 0.75 -0.3 0 30 0');Start-Sleep -Milliseconds 250
  $a=Read-State;$oldIds=@($a.nativeWorldMags|ForEach-Object ref);$before=$a.worldmagRecovery.committed
  Send-XR @('btn a press 180');Start-Sleep -Milliseconds 900
  $part=Wait-NewPart $oldIds;$script:TargetRef=$part.ref;$n=$part.actualCount
  Assert-Equal ($n -gt 0) $true 'Native dropped magazine contains positive real rounds'
  $a=Read-State;Assert-Equal $a.pouch.installedMode 4 'Normal ejection leaves empty magazine well'
  Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') ($inventory-$n) 'Native ejection debits the exact world stack once'
  if($EmptyChamber -and $cycle -eq 0){
   Send-XR @('trigger r 1');Start-Sleep -Milliseconds 180;Send-XR @('trigger r 0');Start-Sleep -Milliseconds 500
   $a=Read-State;Write-Output "CHAMBER before pickup: $($a.chambered), rounds $(Get-Native 'player.GetCurrentAmmoRounds')"
   Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') 0 'Last chambered round fired before magazine recovery'
   $inventory=(Get-Native 'player.GetItemCount (player.GetCurrentAmmo)')+$n
  }
  $chamber=Get-Native 'player.GetCurrentAmmoRounds'
  # Ejected magazines may bounce beyond a real arm's reach. Move the fixture
  # player beside the settled reference, then test ordinary native pickup.
  Start-Sleep -Milliseconds 500
  $a=Read-State;$p=Part $a $part.ref
  if($p -and ([Math]::Abs($p.tracking[0]) -gt 1 -or [Math]::Abs($p.tracking[2]) -gt 1)){
   Send-Game @(("player.MoveTo {0:X8} 0 -35 0" -f [uint32]$part.ref))|Out-Null
   Start-Sleep -Milliseconds 700
  }
  $script:Progress="cycle $cycle normal native grip"
  Grab-NativeWorldMag $part.ref $before
  $a=Read-State;Assert-Equal $a.worldmagRecovery.committed ($before+1) 'One and only one native handoff committed'
  Assert-Equal $a.worldmagRecovery.rounds $n 'Handoff keeps the exact native world count'
  Write-Output "CHAMBER after pickup: $($a.chambered)"
  Assert-Equal $a.pouch.installedMode 2 'Native empty-well initializer produces installed-held mode'
  Assert-Equal $a.pouch.installedRounds $n 'Native installed-held magazine keeps exact count'
  Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Native Activate restores exact stack once'
  Assert-Equal ([bool](Part $a $part.ref)) $false 'Consumed upstream reference is no longer live'
  if($cycle -eq 0) {
   $script:Progress='native well insertion';Insert-NativeRecovered
   if($EmptyChamber){$a=Read-State;Assert-Equal $a.chambered $false 'Recovered magazine insertion leaves empty chamber empty';$loaded=Get-Native 'player.GetCurrentAmmoRounds';Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 500;Assert-Equal (Read-State).chambered $false 'Trigger does not chamber the recovered magazine';Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Trigger cannot spend ammo before cycling the action'}
   if($EmptyChamber){
    $a=Read-State;$handle=$a.tutorial.parts[1].node
    if(!$handle){throw 'Native slide metadata unavailable'}
    for($j=0;$j -lt 3;$j++){
     $a=Read-State;$ss=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$part=@($a.tutorial.parts|Where-Object node -eq $handle)[0];$q=@();for($k=0;$k -lt 3;$k++){$q+=($ss.handL.pos[$k]+$part.tracking[$k]-$a.nativeGripTracking[0][$k])}
     Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 600" -f $q[0],$q[1],$q[2]));Start-Sleep -Milliseconds 750
    }
    Send-XR @('trigger l 1');Start-Sleep -Milliseconds 350
    $a=Read-State;$ss=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$part=@($a.tutorial.parts|Where-Object node -eq $handle)[0];$q=@();for($k=0;$k -lt 3;$k++){$q+=($ss.handL.pos[$k]+$part.pullTracking[$k]-$part.tracking[$k])}
    Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1000" -f $q[0],$q[1],$q[2]));Start-Sleep -Milliseconds 1200
    Send-XR @('trigger l 0','hand l to grip -.3 .75 -.25 0 55 0 700');Start-Sleep -Milliseconds 900
    Assert-Equal (Read-State).chambered $true 'Native physical slide chambers recovered ammunition'
   }
   Assert-Equal (Get-Native 'player.GetCurrentAmmoRounds') ($n+$chamber) 'Loaded rounds equal recovered magazine plus chamber'
  } else {
   $script:Progress='native pouch return';$idsBeforeReturn=@($a.nativeWorldMags|Where-Object valid|ForEach-Object ref)
   Return-NativePouch;$a=Read-State
   Assert-Equal ($a.pouch.installedMode -ne 2 -and $a.pouch.installedMode -ne 3 -and $a.pouch.mode -eq 0) $true 'Native pouch release clears held magazine states'
   $new=@($a.nativeWorldMags|Where-Object { $_.valid -and $idsBeforeReturn -notcontains $_.ref })
   Assert-Equal $new.Count 0 'Pouch return creates no duplicate world magazine'
  }
  Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Insertion or pouch return preserves restored ammo count'
 }
 $a=Read-State;Assert-Equal $a.worldmagRecovery.committed ($initialCommitted+2) 'Two native recoveries committed exactly twice'
 $script:Progress='passed'
 [ordered]@{pass=$true;weapon=$Weapon;rounds=$Rounds;progress=$script:Progress;state=$a}|ConvertTo-Json -Depth 20|Set-Content -LiteralPath $script:ResultPath
 if($Capture){Save-Eyes "native-worldmag-view-$Weapon"}
} catch {
 $errorText=$_.Exception.Message
 [ordered]@{pass=$false;weapon=$Weapon;rounds=$Rounds;progress=$script:Progress;target=$script:TargetRef;error=$errorText;state=$script:LastAddon}|ConvertTo-Json -Depth 20|Set-Content -LiteralPath $script:ResultPath
 throw "Native worldmag test failed at $script:Progress : $errorText (bounded dump: $script:ResultPath)"
}
