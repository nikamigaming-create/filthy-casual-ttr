. "$PSScriptRoot\runtime-helpers.ps1"
function Controls {Send-Game @('sc_controls_state')|Out-Null;Get-Content "$script:Session\controls-state.json" -Raw|ConvertFrom-Json}
& "$PSScriptRoot\vertibird-smoke.ps1"
Send-XR @('input clear','head pose 0 1.2 0 0 0 0')
Park-Hands
Send-Game @('PVHGutsyREF.MoveTo PVHGutsyPost','PVHGutsyREF.Enable','set PVHGeneralQuest.CanFly to 1','StopQuest PVHQuest1')|Out-Null
$car=Get-Native 'player.PlaceAtMe (GetFormFromMod "UmDrivingVIIIb.esp" "00E6BC") 1 550 0'
$id=('{0:X8}' -f $car)
Get-Native ('Call (CompileScript "FCVehicleTest.txt") "'+$id+'" 3')|Out-Null
Start-Sleep -Seconds 2
Assert-Equal (Get-Native 'player.GetDead') 0 'Test character alive'
Send-Game @('save FC_Vehicles_Ready')|Out-Null;Start-Sleep -Seconds 2
Send-Game @('sc_controls')|Out-Null;Start-Sleep -Milliseconds 450
Save-Eyes 'robco-controls-outdoor'
Send-XR @('hand r aimtrim 0 0','hand r grip pose -.43 1.373 -.45 0 0 0');Start-Sleep -Milliseconds 300
Assert-Equal (Controls).pointerRow 36 'Mode selector hit remains correct'
foreach($mode in 'driving','flight'){
 Send-XR @('trigger r 1');Start-Sleep -Milliseconds 100;Send-XR @('trigger r 0');Start-Sleep -Milliseconds 400
 Assert-Equal (Controls).mode ('vehicles.'+$mode) 'Provider can be configured on foot'
 Assert-Equal (Controls).selected 0 'Mode starts with populated actions'
 Save-Eyes ('robco-'+$mode+'-filled')
}
Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350
Park-Hands
Send-Game @('sc_settings')|Out-Null;Start-Sleep -Milliseconds 450;Save-Eyes 'robco-height-outdoor';Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350
Send-Game @('player.AddItem 0000434F 1','player.AddItem 00004353 1','sc_weapons')|Out-Null;Start-Sleep -Milliseconds 450;Save-Eyes 'robco-weapons-outdoor';Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350
Send-Game @('player.AddItem 00015169 5','sc_aid')|Out-Null;Start-Sleep -Milliseconds 450;Save-Eyes 'robco-aid-outdoor';Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350
Write-Output 'PASS: parked car/Vertibird fixture saved; populated provider panels captured in both eyes'
