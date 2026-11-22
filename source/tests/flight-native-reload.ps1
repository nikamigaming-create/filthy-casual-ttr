. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -5 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 7
for($i=0;$i -lt 4 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('set GameHour to 12')|Out-Null
Send-Game @('player.SetPos X -72500','player.SetPos Y 2000')|Out-Null
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
$until=[DateTime]::UtcNow.AddSeconds(35)
do{
 $f=Flight-State;if($f.live -eq 1 -and $f.controlling -eq 1){break}
 if((Get-Addon).menu){Send-XR @('btn a press 150');Start-Sleep -Milliseconds 350}
 Start-Sleep -Milliseconds 250
}while([DateTime]::UtcNow -lt $until)
$f|ConvertTo-Json|Write-Output
Assert-Equal $f.live 1 'Quest inputs are fresh and flight controls enabled'
Assert-Equal $f.controlling 1 'Native takeoff finished and pilot has control'
$script:CaptureAddonPath="D:\TTWVR\MO2\mods\TTWVR - RobCo Vehicle Test\NVSE\Plugins\SeatedControls.dll"
# Independent native magazine test while the right stick flies forward.
. "$PSScriptRoot\casual-guide-helpers.ps1"
Send-XR @('stick r 0 0','btn a down');Start-Sleep -Seconds 4
Send-XR @('btn a up');Start-Sleep -Milliseconds 400
$flightBefore=Flight-State
Assert-Equal ($flightBefore.z -gt $f.z+500) $true 'Climbed clear of terrain before the moving reload'
Write-Output ('Reload start: '+($flightBefore|ConvertTo-Json -Compress))
if(!(Get-Addon).tutorial.enabled){for($i=0;$i -lt 3;$i++){Send-XR @('btn menu press 80');Start-Sleep -Milliseconds 160};Start-Sleep -Milliseconds 500}
Park-Hands
$weapon=Get-Native 'Weap10mmPistol'
Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250
Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Milliseconds 1400
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 600}
$hoverPouch=(Get-Addon).insertion.pouchTarget
Assert-Equal $hoverPouch.Count 3 'Measured native pouch target while hovering'
$cardBefore=Get-Addon
Send-XR @('stick r 0 .4')
& "$PSScriptRoot\tutorial-pistol-reload.ps1" -StageReady -SkipPointing -PouchTarget $hoverPouch -TakeName 'mounted-native-magazine'
$rounds=Get-Native 'player.GetCurrentAmmoRounds'
Send-XR @('trigger r pull 140');Start-Sleep -Milliseconds 600
Assert-Equal ((Get-Native 'player.GetCurrentAmmoRounds') -lt $rounds) $true 'Airborne pistol fires after native magazine reload'
$f=Flight-State
Write-Output ('Reload end: '+($f|ConvertTo-Json -Compress))
Assert-Equal ([Math]::Abs($f.x-$flightBefore.x)+[Math]::Abs($f.y-$flightBefore.y) -gt 30) $true 'Aircraft remains mobile during normal hand and reload use'
Assert-Equal (Get-Native 'MenuMode') 0 'Native reload did not open menu mode'
$cardAfter=Get-Addon
Assert-Equal $cardAfter.tutorialMounted $true 'Tutorial uses current cockpit frame'
Assert-Equal ($cardAfter.tutorialPublications -gt $cardBefore.tutorialPublications+60) $true 'Tutorial publishes with native weapon poses'
$localError=0;for($j=0;$j -lt 3;$j++){$localError+=[Math]::Abs($cardAfter.tutorialCardLocal[$j]-$cardBefore.tutorialCardLocal[$j])}
Assert-Equal ($localError -lt .05) $true 'Reading card retains cockpit-local anchor during travel'
Send-XR @('stick r 0 0')
Save-Eyes 'flight-native-reload-complete'
