. "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null;Start-Sleep -Milliseconds 900}
$a=Get-Addon
Assert-Equal ($a.tutorial.visualElements -gt 0) $true 'On-foot tutorial visible after pose publication'
Assert-Equal $a.tutorialMounted $false 'On-foot card keeps world zone'
$anchor=$a.tutorialCardWorld;$before=$a.tutorialPublications
Start-Take 'tutorial-pose-motion' 20
try{
 Send-XR @('head to -.06 1.13 -.04 -22 -12 0 900','hand r to grip .35 1 -.40 20 65 -10 950');Start-Sleep -Milliseconds 1100
 Send-XR @('head to .06 1.08 .02 22 -18 0 900','hand r to grip .08 .85 -.55 -15 50 10 950');Start-Sleep -Milliseconds 1100
 Send-XR @('head to 0 1.1 0 0 -15 0 1000','hand r to grip .2 .95 -.45 0 55 0 1100');Start-Sleep -Milliseconds 1300
 $a=Get-Addon
 Assert-Equal ($a.tutorialPublications -gt $before+30) $true 'Tutorial follows completed poses during head and hand motion'
 $distance=0;for($j=0;$j -lt 3;$j++){$distance+=[Math]::Abs($a.tutorialCardWorld[$j]-$anchor[$j])}
 Assert-Equal ($distance -lt .05) $true 'Reading card does not chase head or hands'
 Send-Game @('player.SetWeaponOut 0')|Out-Null;Start-Sleep -Milliseconds 700
 Assert-Equal (Get-Addon).tutorial.visualElements 0 'Holstering clears tutorial geometry'
 Finish-Take
}finally{Stop-Take}
