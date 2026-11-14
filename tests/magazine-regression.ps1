. "$PSScriptRoot\magazine-helpers.ps1"
& "$PSScriptRoot\magazine-ejection.ps1"
# Empty part is a real native pickup/throw object too.
$inventory=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
$a=Get-Addon;$partId=$a.physicalMags.lastRef
Grab-PhysicalPart $partId
Assert-Equal (Get-Addon).nativeGrabRef ([uint32]$partId) 'Empty magazine: normal native grip pickup'
Send-XR @('hand l to grip -0.3 1.0 -0.4 0 30 0 600');Start-Sleep -Milliseconds 700
Send-XR @('hand l to grip -0.2 1.1 -0.85 0 30 0 250');Start-Sleep -Milliseconds 180
Send-XR @('grip l 0');Start-Sleep -Milliseconds 600
Assert-Equal (Get-Addon).nativeGrabRef 0 'Empty magazine: native throw release'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Empty pickup and throw create no ammo'
# The saved test contains the loaded magazine. Check cell unload and return.
Send-Game @('load SC_PhysicalParts_QA')|Out-Null;Start-Sleep -Seconds 5
$a=Get-Addon;$ids=@($a.physicalParts|ForEach-Object ref)
Send-Game @('coc GSDocMitchellHouse')|Out-Null;Start-Sleep -Seconds 4
Send-Game @('pcb')|Out-Null;Start-Sleep -Seconds 1
Send-Game @('coc GSGeneralStore')|Out-Null;Start-Sleep -Seconds 4
Assert-Equal @((Get-Addon).physicalParts|Where-Object ref -in $ids).Count $ids.Count 'Fitted appearance returns after native cell unload and return'
$a=Get-Addon
# Fixture's original cell may differ; preserve explicit save/load evidence without guessing its editor ID.
Send-Game @('load SC_PhysicalParts_QA')|Out-Null;Start-Sleep -Seconds 5
$a=Get-Addon
Assert-Equal @($a.physicalParts|Where-Object ref -in $ids).Count $ids.Count 'Fitted appearance returns after cell transitions and normal reload'
Save-Eyes 'magazine-fitted-restored'
# Hand-pulled installed magazine: native grip -> pull -> release.
Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 4
Park-Hands;Send-Game @('sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 0000434F')|Out-Null;Start-Sleep -Seconds 1
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null}
Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0','hand l grip pose -0.4 0.8 -0.3 0 30 0');Start-Sleep -Seconds 1
$a=Get-Addon;$before=$a.physicalMags.installedDrops;$t=$a.installedPartTracking
Send-XR @(("hand l grip pos {0:F5} {1:F5} {2:F5}" -f $t[0],($t[1]-$a.heightOffset),$t[2]),'grip l 1');Start-Sleep -Milliseconds 400
Send-XR @('hand l to grip -0.4 0.8 -0.55 0 30 0 700');Start-Sleep -Milliseconds 900
$a=Get-Addon;$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-hand-pull-before-release.json"
Send-XR @('grip l 0');Start-Sleep -Seconds 2
$a=Get-Addon;$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-hand-pull-after-release.json"
Assert-Equal ($a.physicalMags.installedDrops -gt $before) $true 'Native hand-pulled installed magazine becomes physical'
