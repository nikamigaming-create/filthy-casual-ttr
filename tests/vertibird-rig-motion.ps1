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
Start-Take 'vertibird-classic-pilot' 100
try {
 Mark 'Seated flight deck'
 Save-Eyes 'vertibird-classic-front'
 (Get-Addon)|ConvertTo-Json -Depth 14|Set-Content "$script:Repo\work\vertibird\classic\pilot-fit-state.json"
 Start-Sleep -Seconds 4
 Mark 'Free look right'
 Send-XR @('head to .035 1.1 0 30 -8 0 2200');Start-Sleep -Seconds 4
 Save-Eyes 'vertibird-classic-right'
 Mark 'Free look left'
 Send-XR @('head to -.035 1.1 0 -30 -8 0 3000');Start-Sleep -Seconds 4
 Save-Eyes 'vertibird-classic-left'
 Mark 'Overhead instruments and roof mounting'
 Send-XR @('head to 0 1.1 0 25 55 0 1800');Start-Sleep -Seconds 3;Save-Eyes 'vertibird-overhead-mount'
 Mark 'Rear cabin fit'
 Send-XR @('head to 0 1.1 0 150 -5 0 2800');Start-Sleep -Seconds 4;Save-Eyes 'vertibird-rear-cabin'
 Mark 'Back to the flight deck' 
 Send-XR @('head to 0 1.1 0 0 -5 0 3500');Start-Sleep -Seconds 4
 Mark 'Pilot seat and footwell'
 Send-XR @('head to 0 1.1 0 0 -65 0 1800');Start-Sleep -Seconds 2;Save-Eyes 'vertibird-pilot-footwell'
 Send-XR @('head to 0 1.1 0 0 -5 0 1800');Start-Sleep -Seconds 2
 $pose=Get-Content "$script:Session\mounted-pose.json" -Raw|ConvertFrom-Json
 $pose|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\work\vertibird\classic\seated-pose-proof.json"
 Assert-Equal ($pose.samples -gt 60 -and $pose.applies -gt 60 -and $pose.sourceTime -gt .1) $true 'Live native seated animation advances'
 foreach($side in 'L','R'){
  Assert-Equal ($pose.bones."Bip01 $side Calf"[1] -gt $pose.bones.'Bip01 Pelvis'[1]+10) $true "$side knee faces forward in the footwell"
  Assert-Equal ($pose.bones."Bip01 $side Foot"[2] -gt 485 -and $pose.bones."Bip01 $side Foot"[2] -lt 515) $true "$side foot stays near the cabin floor"
 }

 Mark 'Climb above the Mojave'
 $before=Flight-State
 Send-XR @('btn a down');Start-Sleep -Seconds 4;Send-XR @('btn a up')
 Assert-Equal ((Flight-State).z -gt $before.z+100) $true 'Native climb works with classic Vertibird cockpit'
 Start-Sleep -Seconds 2
 Mark 'Forward flight'
 $before=Flight-State
 Send-XR @('stick l 0 .5');Start-Sleep -Seconds 5;Send-XR @('stick l 0 0')
 $after=Flight-State
 Assert-Equal ([Math]::Abs($after.x-$before.x)+[Math]::Abs($after.y-$before.y) -gt 30) $true 'Aircraft moves through the real world'
 Save-Eyes 'vertibird-classic-flight'
 Start-Sleep -Seconds 3
 Mark 'Turn the aircraft with free head look'
 Send-XR @('stick r .5 0','head to 0 1.1 0 -20 -5 0 1200');Start-Sleep -Seconds 2
 Send-XR @('stick r 0 0','head to 0 1.1 0 0 -5 0 1200');Start-Sleep -Seconds 2
 Mark 'Native landing'
 Send-XR @('btn b press 350')
 $deadline=[DateTime]::UtcNow.AddSeconds(30)
 do {Start-Sleep -Seconds 1;$piloting=Get-Native 'GetVariable "Piloting" (GetFormFromMod "Player Vertibird.esm" "000AE7")'}while($piloting -eq 1 -and [DateTime]::UtcNow -lt $deadline)
 Assert-Equal $piloting 0 'Native landing completes'
 Assert-Equal (Get-Native 'player.GetDead') 0 'Pilot survives'
 Assert-Equal (Get-Native 'GetPlayerControlsDisabled') 0 'On-foot controls restored'
 Mark 'Back on foot'
 Start-Sleep -Seconds 3
 Finish-Take
} finally {Stop-Take}
