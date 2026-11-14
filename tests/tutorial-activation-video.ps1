& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\tutorial-development';$script:events=[Collections.Generic.List[object]]::new()
Send-XR @('hand r to grip .10 .90 -.50 35 55 0 1200');Start-Sleep -Milliseconds 1500
$manual=(Get-Addon).manualReload
for($i=0;$i -lt 3;$i++){Send-XR @('btn menu press 80');Start-Sleep -Milliseconds 160}
Start-Sleep -Seconds 3;Assert-Equal (Get-Addon).tutorial.enabled $false 'Tutorial can be turned off without unequipping'
Start-Take 'tutorial-activation' 40
try{
 Mark 'Triple-tap left Menu to show the guide';Start-Sleep -Seconds 2
 for($i=0;$i -lt 3;$i++){Send-XR @('btn menu press 80');Start-Sleep -Milliseconds 160}
 Start-Sleep -Seconds 3
 Assert-Equal (Get-Addon).tutorial.enabled $true 'Triple Menu enables actual in-world guide'
 Assert-Equal (Get-Addon).manualReload $manual 'Triple Menu does not toggle reload mode'
 Save-Eyes 'tutorial-unlit-enabled'
 Mark 'One tap changes reload mode'
 Send-XR @('btn menu press 80');Start-Sleep -Seconds 3
 Assert-Equal (Get-Addon).manualReload (!$manual) 'Single Menu retains reload toggle'
 Save-Eyes 'tutorial-unlit-automatic'
 Mark 'Tap again for manual reload; the guide stays available'
 Send-XR @('btn menu press 80');Start-Sleep -Seconds 3
 Assert-Equal (Get-Addon).manualReload $manual 'Single Menu returns to original reload mode'
 Save-Eyes 'tutorial-unlit-manual'
 Finish-Take
}finally{Stop-Take}
foreach($editor in 'WeapNVHuntingShotgun','WeapPlasmaRifle'){
 Park-Hands;$weapon=Get-Native $editor
 Send-Game @(('player.AddItem {0:X8} 1' -f [uint32]$weapon),'sc_weapons')|Out-Null;Start-Sleep -Milliseconds 200
 Send-Game @(('sc_pick {0:X8}' -f [uint32]$weapon))|Out-Null;Start-Sleep -Milliseconds 1000
 Send-XR @('hand r grip pose .10 .90 -.50 35 55 0','hand l grip pose -.35 .65 -.15 0 55 0','trigger r pull 140');Start-Sleep -Milliseconds 1000
 Save-Eyes ('tutorial-unlit-'+$editor)
 Send-XR @('head to .25 1.1 .05 25 -25 0 1000');Start-Sleep -Milliseconds 1200
 Save-Eyes ('tutorial-unlit-'+$editor+'-lean')
 Send-XR @('head to .04 1.1 0 10 -20 0 1000');Start-Sleep -Milliseconds 1200
}
