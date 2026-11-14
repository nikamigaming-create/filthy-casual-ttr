& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\runtime-helpers.ps1"
$records=@()
foreach($editor in 'WeapNV44Revolver','WeapNVCaravanShotgun') {
 Park-Hands
 $weapon=Get-Native $editor
 Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 200
 Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Milliseconds 1000
 Send-XR @('hand r grip pose .10 .90 -.50 35 55 0','hand l grip pose -.35 .65 -.15 0 55 0');Start-Sleep -Milliseconds 400
 $records+=@{editor=$editor;motion='initial';state=(Get-Addon)}
 $motions=@(@(35,55,75),@(35,55,75),@(35,55,-75),@(35,115,0),@(35,-10,0),@(100,55,0),@(-30,55,0))
 $index=0
 foreach($p in $motions) {
  Send-XR @('btn a down');Start-Sleep -Milliseconds 120
  Send-XR @(("hand r to grip .10 .90 -.50 {0} {1} {2} 180" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 300
  Send-XR @('btn a up');Start-Sleep -Milliseconds 100
  Send-XR @('hand r to grip .10 .90 -.50 35 55 0 600');Start-Sleep -Milliseconds 750
  $a=Get-Addon
  $records+=@{editor=$editor;motion=($p -join ',');state=$a}
  $records|ConvertTo-Json -Depth 22|Set-Content "$script:Repo\evidence\tutorial-gate-motion-probe.json"
  Save-Eyes ("tutorial-gate-"+$editor+'-'+$index)
  Write-Output ("MOTION "+$editor+' '+($p -join ',')+' rounds='+$a.tutorial.rounds)
  $index++
 }
}
