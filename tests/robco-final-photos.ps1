. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.2 0 0 0 0','btn a press 150')
Send-Game @('load FC_Vehicles_Ready')|Out-Null;Start-Sleep -Seconds 5
for($i=0;$i -lt 5 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 150');Start-Sleep -Milliseconds 350}
Park-Hands
Send-Game @('player.SetPos X -54500','player.SetAngle Z 270','sc_controls')|Out-Null;Start-Sleep -Milliseconds 500
Save-Eyes 'robco-controls-final'
foreach($mode in 'driving','flight'){
 Send-XR @('hand l aimtrim 0 0','hand l grip pose -.43 1.373 -.45 0 0 0');Start-Sleep -Milliseconds 250
 Send-XR @('trigger l 1');Start-Sleep -Milliseconds 100;Send-XR @('trigger l 0');Start-Sleep -Milliseconds 250
 Park-Hands;Start-Sleep -Milliseconds 300;Save-Eyes ('robco-'+$mode+'-final')
}
Send-XR @('btn b press 150');Start-Sleep -Milliseconds 300
Send-Game @('player.AddItem 0000434F 1','sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250;Send-Game @('sc_pick 0000434F')|Out-Null;Start-Sleep -Seconds 1
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 250}
Send-XR @('hand r to grip .25 .95 -.45 0 55 0 500','hand l to grip -.25 .85 -.35 0 20 0 500');Start-Sleep -Milliseconds 650
for($i=0;$i -lt 3;$i++){Send-XR @('btn menu down');Start-Sleep -Milliseconds 80;Send-XR @('btn menu up');Start-Sleep -Milliseconds 100}
Start-Sleep -Seconds 1
$a=Get-Addon;Assert-Equal $a.tutorial.enabled $true 'Tutorial toggles with native Menu sequence'
Assert-Equal ($a.tutorial.visualElements -gt 0) $true 'Populated RobCo tutorial card is present'
Save-Eyes 'robco-tutorial-final'
Write-Output 'PASS final populated controls and tutorial capture'
