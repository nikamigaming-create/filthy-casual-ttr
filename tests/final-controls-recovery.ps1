. "$PSScriptRoot\runtime-helpers.ps1"
& "$PSScriptRoot\native-worldmag-runtime.ps1" -Weapon 0x434F -Rounds 6 -Capture
Copy-Item -LiteralPath "$script:Repo\evidence\native-worldmag-runtime.json" -Destination "$script:Repo\evidence\native-worldmag-pistol.json" -Force
& "$PSScriptRoot\native-worldmag-runtime.ps1" -Weapon 0x8F21E -Rounds 12 -Capture
Copy-Item -LiteralPath "$script:Repo\evidence\native-worldmag-runtime.json" -Destination "$script:Repo\evidence\native-worldmag-carbine.json" -Force
Send-XR @('input clear','head pose 0 1.2 0 0 0 0')
Send-Game @('load FC_Vehicles_Ready')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 6 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 150');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('player.SetWeaponOut 0','set GameHour to 12','player.MoveTo 00145D50')|Out-Null;Start-Sleep -Seconds 4
for($i=0;$i -lt 6 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 150');Start-Sleep -Milliseconds 400}
$ground=Get-Native 'GetTerrainHeight -54000 -78600'
Send-Game @('player.SetPos X -54000','player.SetPos Y -78600',("player.SetPos Z "+($ground+10)),'player.SetAngle Z 0')|Out-Null;Start-Sleep -Seconds 3
Assert-Equal (Get-Native 'Call (CompileScript "FCStageParked.txt")') 1 'Native landed aircraft relocated without changing its state'
$ground=Get-Native 'GetTerrainHeight -54000 -77000'
Get-Native ('Call (CompileScript "FCVertibirdGround.txt") '+($ground-40))|Out-Null
$car=Get-Native 'player.PlaceAtMe (GetFormFromMod "UmDrivingVIIIb.esp" "00E6BC") 1 400 0'
$id=('{0:X8}' -f $car);Get-Native ('Call (CompileScript "FCVehicleTest.txt") "'+$id+'" 3')|Out-Null
Start-Sleep -Seconds 2
Assert-Equal (Get-Native 'player.GetDead') 0 'Character ready in the vehicle test field'
Send-Game @('save FC_Vehicles_Ready')|Out-Null
# Move to a clear patch for UI screenshots; this unsaved move does not alter the test save.
Send-Game @('player.SetPos X -54500','player.SetAngle Z 270')|Out-Null;Start-Sleep -Seconds 1
function Controls {Send-Game @('sc_controls_state')|Out-Null;Get-Content "$script:Session\controls-state.json" -Raw|ConvertFrom-Json}
Send-Game @('sc_controls')|Out-Null;Start-Sleep -Milliseconds 450;Save-Eyes 'robco-controls-final'
Send-XR @('hand r aimtrim 0 0','hand r grip pose -.43 1.373 -.45 0 0 0');Start-Sleep -Milliseconds 300
Assert-Equal (Controls).pointerRow 36 'Mode button accessible in final candidate'
foreach($mode in 'driving','flight'){Send-XR @('trigger r 1');Start-Sleep -Milliseconds 100;Send-XR @('trigger r 0');Start-Sleep -Milliseconds 400;Assert-Equal (Controls).mode ('vehicles.'+$mode) 'Inactive mode remains editable';Save-Eyes ('robco-'+$mode+'-final')}
Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350;Park-Hands
Send-Game @('sc_settings')|Out-Null;Start-Sleep -Milliseconds 450;Save-Eyes 'robco-height-final';Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350
Send-Game @('player.AddItem 0000434F 1','player.AddItem 00004353 1','sc_weapons')|Out-Null;Start-Sleep -Milliseconds 450;Save-Eyes 'robco-weapons-final';Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350
Send-Game @('player.AddItem 00015169 5','sc_aid')|Out-Null;Start-Sleep -Milliseconds 450;Save-Eyes 'robco-aid-final';Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350
Write-Output 'PASS: pistol/carbine native recovery, reinsertion and pouch; field test save; final filled RobCo captures'
