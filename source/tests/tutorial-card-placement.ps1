& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\runtime-helpers.ps1"
$a=Get-Addon;$anchor=$a.tutorialCardWorld
Send-XR @('hand r curve grip -.05 1.20 -.30 55 95 0 1300 0 .05 0');Start-Sleep -Milliseconds 1600
$b=Get-Addon;$d=0;for($j=0;$j -lt 3;$j++){$d+=[Math]::Pow($anchor[$j]-$b.tutorialCardWorld[$j],2)}
Assert-Equal ([Math]::Sqrt($d) -lt .001) $true 'Instruction card stays anchored while weapon moves toward the face'
Save-Eyes 'tutorial-card-weapon-close'
Send-XR @('hand r to grip .10 .90 -.50 35 55 0 1200','head to .25 1.1 .05 25 -25 0 1200');Start-Sleep -Milliseconds 1600
$c=Get-Addon;$d=0;for($j=0;$j -lt 3;$j++){$d+=[Math]::Pow($anchor[$j]-$c.tutorialCardWorld[$j],2)}
Assert-Equal ([Math]::Sqrt($d) -lt .001) $true 'Instruction card stays anchored through a small head lean'
Save-Eyes 'tutorial-card-head-lean'
