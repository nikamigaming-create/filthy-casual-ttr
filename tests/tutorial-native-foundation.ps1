. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -15 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 6
for($n=0;$n -lt 3 -and (Get-Addon).nativeTopMenu -eq 1001;$n++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 500}
Park-Hands
function Triple-Menu {
 for($n=0;$n -lt 3;$n++){Send-XR @('btn menu press 80');Start-Sleep -Milliseconds 160}
 Start-Sleep -Milliseconds 600
}
$before=(Get-Addon).manualReload
Triple-Menu
$a=Get-Addon
Assert-Equal $a.tutorial.enabled $true 'Triple Menu enables optional tutorial'
Assert-Equal $a.manualReload $before 'Triple Menu does not change reload mode'
Assert-Equal $a.tutorial.metadataCount 583 'Native installed metadata loaded in game'
$records=@()
foreach($editor in @('Weap10mmPistol','WeapNVAssaultCarbine','WeapPlasmaRifle','WeapHuntingRifle','WeapNV357Revolver','WeapNVHuntingShotgun','WeapNVCaravanShotgun')){
 $weapon=Get-Native $editor
 Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250
 Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Seconds 1
 Send-XR @('hand r grip pose .25 .95 -.43 0 55 0','hand l grip pose -.3 .75 -.23 0 55 0')
 Start-Sleep -Milliseconds 400
 $a=Get-Addon
 Assert-Equal $a.tutorial.weapon $weapon 'Tutorial follows actual equipped weapon'
 Assert-Equal $a.tutorial.valid $true 'Native reload root is valid'
 Assert-Equal $a.tutorial.editor $editor.ToLowerInvariant() 'Live EditorID selects measured geometry'
 foreach($p in $a.tutorial.parts){Assert-Equal $p.found $true ('Actual native part exists: '+$p.node)}
 $records+=@{editor=$editor;state=$a}
 $records|ConvertTo-Json -Depth 15|Set-Content "$script:Repo\evidence\tutorial-native-foundation.json"
 Save-Eyes ("tutorial-part-"+$editor)
 $a.tutorial|ConvertTo-Json -Depth 6 -Compress
 if($editor -eq 'Weap10mmPistol'){
  if(!$a.manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 400}
  Send-XR @('btn a press 180');Start-Sleep -Milliseconds 800
  $a=Get-Addon
  Assert-Equal $a.pouch.installedMode 4 'Native magazine ejection leaves empty well'
  Assert-Equal $a.tutorial.step 'fetch-ammo' 'Guide reacts to real magazine ejection'
  $records+=@{editor='pistol-empty-well';state=$a}
 }
}
$before=(Get-Addon).manualReload
Triple-Menu
Assert-Equal (Get-Addon).tutorial.enabled $false 'Triple Menu disables tutorial'
Assert-Equal (Get-Addon).manualReload $before 'Turning guide off leaves reload mode unchanged'
$records|ConvertTo-Json -Depth 15|Set-Content "$script:Repo\evidence\tutorial-native-foundation.json"
Save-Eyes 'tutorial-outdoor-stage-probe'

