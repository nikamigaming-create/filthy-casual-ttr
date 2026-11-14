$ErrorActionPreference='Stop'
& "$PSScriptRoot\tutorial-visual-smoke.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
. "$PSScriptRoot\tutorial-point-helper.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\tutorial-development'
New-Item -ItemType Directory -Force -Path $script:Guide|Out-Null
$script:events=[Collections.Generic.List[object]]::new()
Start-Sleep -Seconds 3
$before=Get-Addon
Start-Take 'pointing-second-pass' 40
try{
 Mark 'Point to the slide'
 Point-TutorialPart '##Slide'
 Save-Eyes 'tutorial-point-slide-v2'
 Mark 'Point to the magazine'
 Point-TutorialPart '##Clip'
 Save-Eyes 'tutorial-point-magazine-v2'
 $a=Get-Addon
 Assert-Equal $a.pouch.installedMode $before.pouch.installedMode 'Pointing does not eject or insert anything'
 Assert-Equal $a.nativeGrabRef 0 'Pointing does not pick up an item'
 $a|ConvertTo-Json -Depth 14|Set-Content "$script:Repo\evidence\tutorial-point-final.json"
 Finish-Take
}finally{Stop-Take}
