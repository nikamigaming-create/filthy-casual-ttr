. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -5 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 7
for($i=0;$i -lt 4 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('tdetect','set GameHour to 12')|Out-Null
Send-Game @('player.SetPos X -72500','player.SetPos Y 2000','player.SetAngle Z 90')|Out-Null
Get-Native 'Call (CompileScript "FCVertibirdTest.txt") 2'|Out-Null
Start-Sleep -Seconds 3
foreach($name in 'Player Vertibird.esm','Player Vertibird TTW Patch.esp','Vertibird Player Home - pilot inside.esp'){Assert-Equal (Get-Native ('IsModLoaded "'+$name+'"')) 1 ($name+' loaded')}
Assert-Equal ((Get-Native 'GetFormFromMod "Vertibird Player Home - pilot inside.esp" "000C20"') -ne 0) $true 'Pilot seat reference resolves'
Send-Game @('coc PVHVertibirdInteriorCell')|Out-Null;Start-Sleep -Seconds 4
# Interior already captured in the dependency smoke check.
Get-Native 'Call (CompileScript "FCVertibirdTest.txt") 0'|Write-Output
Start-Sleep -Seconds 2
# Pilot seat already captured.
Get-Native 'Call (CompileScript "FCVertibirdTest.txt") 1'|Write-Output
Start-Sleep -Seconds 2
# Prompt already captured.
(Get-Addon)|ConvertTo-Json -Depth 14|Set-Content "$script:Repo\work\vertibird\prompt-state.json"
Send-XR @('btn a press 180');Start-Sleep -Seconds 12
Assert-Equal (Get-Native 'GetVariable "Piloting" (GetFormFromMod "Player Vertibird.esm" "000AE7")') 1 'Native pilot seat starts flight'
function Flight-State {Get-Content "$script:Session\flight-state.json" -Raw|ConvertFrom-Json}
$f=Flight-State;$f|ConvertTo-Json|Write-Output
Assert-Equal $f.live 1 'Quest inputs are fresh and flight controls enabled'
Write-Output ('MOVEMENT LOCK '+(Get-Native 'GetPlayerControlsDisabled 1 0 0 0 0 0 0'))

. "$PSScriptRoot\casual-guide-helpers.ps1"
$script:CaptureAddonPath='D:\TTWVR\MO2\mods\TTWVR - Vertibird Development\NVSE\Plugins\SeatedControls.dll'
Send-XR @('btn y press 180','hand l to grip -0.25 .67 -.22 -8 50 -8 1500','hand r to grip .22 .67 -.22 8 50 8 1500','thumbrest l on','thumbrest r on','touch l index on','touch r index on')
Start-Sleep -Seconds 2
Start-Take 'vertibird-hand-parity-after' 65
try {
 Mark 'Fixed head and hands stationary';Start-Sleep -Seconds 5
 Mark 'Climb fixed hands';Send-XR @('btn a down');Start-Sleep -Seconds 4;Send-XR @('btn a up');Start-Sleep -Seconds 2
 Mark 'Forward fixed hands';Send-XR @('stick l 0 .5');Start-Sleep -Seconds 6;Send-XR @('stick l 0 0');Start-Sleep -Seconds 3
 Mark 'Reverse fixed hands';Send-XR @('stick l 0 -.35');Start-Sleep -Seconds 4;Send-XR @('stick l 0 0');Start-Sleep -Seconds 3
 Mark 'Turn with fixed hands';Send-XR @('stick r .5 0');Start-Sleep -Seconds 3;Send-XR @('stick r 0 0');Start-Sleep -Seconds 3
 Mark 'Head and hand opposing motion';Send-XR @('head to 0 1.1 0 -20 -5 0 2400','hand r to grip .30 .8 -.35 -10 45 10 2400');Start-Sleep -Seconds 4
 Mark 'Return';Send-XR @('head to 0 1.1 0 0 -5 0 2200','hand r to grip .22 .67 -.22 8 50 8 2200');Start-Sleep -Seconds 3
 Save-Eyes 'vertibird-hand-parity';Finish-Take
}finally{Stop-Take}
