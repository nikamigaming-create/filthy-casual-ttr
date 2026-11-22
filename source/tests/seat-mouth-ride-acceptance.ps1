. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','profile touch','head pose 0 1.1 0 0 0 0','btn a press 180')
Send-Game @('load FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
$global:FCSeatAcceptanceGround=(Get-Addon).groundHeightOffset
Send-Game @('tgm','player.SetWeaponOut 0','player.AddItem 001613C5 2','sc_aid')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 001613C5')|Out-Null;Start-Sleep -Milliseconds 1400
Send-XR @('hand r to grip .45 .65 -.65 0 0 0 500');Start-Sleep -Milliseconds 600
foreach($pose in @(@(0,0),@(35,30),@(-35,-30))){
 Send-XR @(("head to 0 1.1 0 {0} {1} 0 500" -f $pose[0],$pose[1]));Start-Sleep -Milliseconds 650
 $a=Get-Addon;$distance=0;for($j=0;$j -lt 3;$j++){$distance+=[Math]::Pow($a.mouthTracking[$j]-$a.trackedHead[$j],2)}
 Assert-Equal ([Math]::Abs([Math]::Sqrt($distance)-.117) -lt .025) $true 'Mouth follows eye pose through head turns'
 Assert-Equal ($a.mouthTracking[1] -lt $a.trackedHead[1]) $true 'Mouth remains below eyes'
}
$initial=Get-Native 'player.GetItemCount 001613C5'
for($i=0;$i -lt 5;$i++){
 $a=Get-Addon;if(!$a.heldFood){break};$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$q=@();for($j=0;$j -lt 3;$j++){$q+=($s.handR.pos[$j]+$a.mouthTracking[$j]-$a.heldTracking[$j])}
 Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 0 0 0 550" -f $q[0],$q[1],$q[2]));Start-Sleep -Milliseconds 950
}
Assert-Equal (Get-Addon).heldFood $false 'Kebab consumed after head movement'
Assert-Equal (Get-Native 'player.GetItemCount 001613C5') $initial 'Held kebab consumed once without inventory loss'
function Mode-Check([string]$name,[string]$capture){
 $mode=if($name -eq 'vehicles.driving'){1}else{2};$delta=if($mode -eq 1){.05}else{-.05}
 $a=Get-Addon;Assert-Equal $a.seatContext $mode 'Seat settings follow actual native mounted mode'
 Send-Game @('sc_settings')|Out-Null;Start-Sleep -Milliseconds 350
 Send-XR @('btn x press 120');Start-Sleep -Milliseconds 200
 Send-XR @(("stick r 0 {0}" -f [Math]::Sign($delta)));Start-Sleep -Milliseconds 120;Send-XR @('stick r center');Start-Sleep -Milliseconds 200
 Assert-Equal ([Math]::Abs((Get-Addon).heightOffset-$delta) -lt .001) $true 'Only this seat height adjusted'
 Save-Eyes ("seat-context-"+$mode)
 Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350
 Assert-Equal (Get-Addon).seatContext $mode 'Saving height does not dismount'
 Assert-Equal ([Math]::Abs((Get-Addon).groundHeightOffset-$global:FCSeatAcceptanceGround) -lt .001) $true 'Mounted setting preserves ground calibration'
}
& "$PSScriptRoot\vehicle-drive.ps1"
Assert-Equal (Get-Addon).seatContext 0 'Car exit restores ground context'
Assert-Equal ([Math]::Abs((Get-Addon).heightOffset-$global:FCSeatAcceptanceGround) -lt .001) $true 'Car exit restores ground height'
& "$PSScriptRoot\vertibird-smoke.ps1"
Assert-Equal (Get-Addon).seatContext 0 'Landing restores ground context'
$qa='D:\TTWVR\MO2\mods\TTWVR - RobCo Vehicle Test\NVSE\Plugins\SeatedControls.ini'
$ini=Get-Content $qa -Raw
Assert-Equal ($ini -match '(?m)^CarSeatOffsetCm=5\.0') $true 'Car offset persisted separately'
Assert-Equal ($ini -match '(?m)^FlightSeatOffsetCm=-5\.0') $true 'Flight offset persisted separately'
& "$PSScriptRoot\bighorner-challenge-video.ps1"
