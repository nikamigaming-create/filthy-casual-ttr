. "$PSScriptRoot\runtime-helpers.ps1"
Park-Hands
$weapon=Get-Native 'WeapHuntingRifle'
Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250
Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Milliseconds 1400
Send-XR @('hand r to grip .1 .85 -.48 35 55 0 800','hand l to grip -.35 .65 -.15 0 55 0 800');Start-Sleep -Milliseconds 1000
$ammo=Get-Native 'player.GetCurrentAmmo';Send-Game @(('player.AddItem {0:X8} 60' -f [uint32]$ammo))|Out-Null
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 300}
if(!(Get-Addon).tutorial.enabled){for($i=0;$i -lt 3;$i++){Send-XR @('btn menu press 80');Start-Sleep -Milliseconds 160};Start-Sleep -Milliseconds 500}
Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 600
$a=Get-Addon
Assert-Equal $a.tutorial.valid $true 'Mounted native gun metadata is available'
$part=@($a.tutorial.parts|Where-Object node -eq '##HRBolt')[0]
Assert-Equal ($null -ne $part) $true 'Mounted native rifle bolt exists'
for($i=0;$i -lt 3;$i++){
 $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$part=@($a.tutorial.parts|Where-Object node -eq '##HRBolt')[0]
 $p=@();for($j=0;$j -lt 3;$j++){$p+=($s.handL.pos[$j]+$part.tracking[$j]-$a.nativeGripTracking[0][$j])}
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 600" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 750
}
$a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$start=$s.handL.pos;$part=@($a.tutorial.parts|Where-Object node -eq '##HRBolt')[0]
$end=@();for($j=0;$j -lt 3;$j++){$end+=($start[$j]+$part.pullTracking[$j]-$part.tracking[$j])}
Send-XR @('trigger l 1');Start-Sleep -Milliseconds 250
Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1000" -f $end[0],$end[1],$end[2]));Start-Sleep -Milliseconds 1200
Assert-Equal (Get-Native 'MenuMode') 0 'Offhand bolt manipulation does not open a menu'
Save-Eyes 'mounted-native-bolt-back'
Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} -4 55 -6 1000" -f $start[0],$start[1],$start[2]));Start-Sleep -Milliseconds 1200
Send-XR @('trigger l 0','hand l to grip -.30 .65 -.20 0 55 0 700');Start-Sleep -Milliseconds 950
Assert-Equal (Get-Addon).tutorial.chambered $true 'Mounted native offhand bolt cycle chambers a round'
$rounds=Get-Native 'player.GetCurrentAmmoRounds'
Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 600
Assert-Equal ((Get-Native 'player.GetCurrentAmmoRounds') -lt $rounds) $true 'Mounted native weapon fires after cocking'
# Return to the ordinary automatic setting used by this isolated fixture.
Send-Game @('sc_reload')|Out-Null
