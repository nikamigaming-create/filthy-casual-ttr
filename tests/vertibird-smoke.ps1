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
if(Get-Command Mode-Check -ErrorAction SilentlyContinue){Mode-Check 'vehicles.flight' 'robco-flight-active'}
Save-Eyes 'vertibird-flight-idle'
$start=Flight-State
Send-XR @('stick r 0 0.5');$until=[DateTime]::UtcNow.AddSeconds(4)
do{Start-Sleep -Milliseconds 300;$move=Flight-State;$distance=[Math]::Abs($move.x-$start.x)+[Math]::Abs($move.y-$start.y)}while($distance -le 30 -and [DateTime]::UtcNow -lt $until)
Write-Output "Flight displacement=$distance"
Assert-Equal ($move.sy -ge .49) $true 'Quest stick reaches native forward query'
Assert-Equal ([Math]::Abs($move.x-$start.x)+[Math]::Abs($move.y-$start.y) -gt 30) $true 'Native flight actually moves the aircraft'
Send-XR @('stick r 0 0','btn a down');$until=[DateTime]::UtcNow.AddSeconds(4)
do{Start-Sleep -Milliseconds 300;$rise=Flight-State}while($rise.z -le $move.z+100 -and [DateTime]::UtcNow -lt $until)
Write-Output "Flight climb=$($rise.z-$move.z) buttons=$($rise.buttons) controlling=$($rise.controlling)"
Send-XR @('btn a up')
Assert-Equal ($rise.z -gt $move.z+100) $true 'Quest A raises the aircraft through native flight logic'
Save-Eyes 'vertibird-flight-moving'
Send-XR @('btn b press 350');Start-Sleep -Seconds 2
Assert-Equal ((Flight-State).landing -gt 0) $true 'Quest B requests native landing'
$deadline=[DateTime]::UtcNow.AddSeconds(30)
do {Start-Sleep -Seconds 1;$piloting=Get-Native 'GetVariable "Piloting" (GetFormFromMod "Player Vertibird.esm" "000AE7")'}while($piloting -eq 1 -and [DateTime]::UtcNow -lt $deadline)
Assert-Equal $piloting 0 'Native landing completes'
Assert-Equal (Get-Native 'player.GetDead') 0 'Pilot survives the landing'
Assert-Equal (Get-Native 'GetPlayerControlsDisabled') 0 'Native landing restores on-foot controls'
Save-Eyes 'vertibird-flight-landing'
