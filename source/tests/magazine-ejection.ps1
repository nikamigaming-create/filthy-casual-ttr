. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -20 0','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 500}
Park-Hands
Send-Game @('sc_weapons')|Out-Null
Start-Sleep -Milliseconds 400
Send-Game @('sc_pick 0000434F')|Out-Null
Start-Sleep -Seconds 1
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null}
Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0','hand l grip pose -0.6 0.8 -0.4 0 0 0')
Start-Sleep -Seconds 1
$before=Get-Addon
Assert-Equal $before.weaponHand 1 'Test pistol equipped in right hand'
$inventory=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
Send-XR @('btn a press 180')
Start-Sleep -Seconds 2
$a=Get-Addon
$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-ejection-state.json"
$a.physicalMags|ConvertTo-Json -Compress
$a.pouch|ConvertTo-Json -Compress -Depth 5
Assert-Equal ($a.physicalMags.successes -gt $before.physicalMags.successes) $true 'Native magazine eject creates physical part'
$id=('{0:X8}' -f [uint32]$a.physicalMags.lastRef)
$worldRounds=@($a.physicalParts|Where-Object ref -eq $a.physicalMags.lastRef)[0].stackCount
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') ($inventory-$worldRounds) 'Ejected stack debited exactly once (native chamber retained)'
Write-Output "World magazine rounds=$worldRounds; chamber=$(Get-Native 'player.GetCurrentAmmoRounds')"
Send-Game @('save SC_PhysicalParts_QA')|Out-Null
Start-Sleep -Seconds 1
Send-Game @('load SC_PhysicalParts_QA')|Out-Null
Start-Sleep -Seconds 5
$a=Get-Addon
$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-loaded-state.json"
Assert-Equal @($a.physicalParts).Count 1 'Dropped magazine appearance restores after normal save/load'
Save-Eyes 'magazine-after-save-load'
# Empty-magazine variant, using the same native eject control.
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 4
Park-Hands
Send-Game @('sc_weapons')|Out-Null
Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 0000434F')|Out-Null
Start-Sleep -Seconds 1
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null}
Send-Game @('player.SetCurrentAmmoRounds 0')|Out-Null
Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0','hand l grip pose -0.6 0.8 -0.4 0 0 0')
Start-Sleep -Seconds 1
$before=Get-Addon;$inventory=Get-Native 'player.GetItemCount (player.GetCurrentAmmo)'
Send-XR @('btn a press 180');Start-Sleep -Seconds 2
$a=Get-Addon
$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-empty-state.json"
Assert-Equal ($a.physicalMags.emptyDrops -gt $before.physicalMags.emptyDrops) $true 'Native empty-magazine eject creates physical empty part'
Assert-Equal (Get-Native 'player.GetItemCount (player.GetCurrentAmmo)') $inventory 'Empty magazine creates no ammo and removes none'
Assert-Equal @($a.physicalParts).Count 1 'Empty part has attached native magazine appearance' 
Save-Eyes 'magazine-native-ejected'
