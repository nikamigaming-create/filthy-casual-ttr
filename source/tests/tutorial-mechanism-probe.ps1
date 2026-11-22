& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\runtime-helpers.ps1"
$records=@()
foreach($editor in 'WeapNVAssaultCarbine','WeapPlasmaRifle','WeapHuntingRifle','WeapNV44Revolver','WeapNV357Revolver','WeapNVHuntingShotgun','WeapNVCaravanShotgun'){
 Park-Hands
 $weapon=Get-Native $editor
 Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250
 Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Milliseconds 1200
 Send-XR @('hand r grip pose .10 .90 -.50 35 55 0','hand l grip pose -.35 .65 -.15 0 55 0');Start-Sleep -Milliseconds 400
 $before=Get-Addon
 Send-XR @('btn a down');Start-Sleep -Milliseconds 150
 Send-XR @('hand r to grip .10 .90 -.50 35 55 75 250');Start-Sleep -Milliseconds 350
 Send-XR @('btn a up');Start-Sleep -Milliseconds 150
 Send-XR @('hand r to grip .10 .90 -.50 35 55 0 500');Start-Sleep -Milliseconds 700
 $after=Get-Addon
 $records+=@{editor=$editor;before=$before;after=$after}
 $records|ConvertTo-Json -Depth 20|Set-Content "$script:Repo\evidence\tutorial-mechanism-probe.json"
 $after.tutorial|ConvertTo-Json -Depth 10 -Compress|Write-Output
 Save-Eyes ("tutorial-mechanism-"+$editor)
}
