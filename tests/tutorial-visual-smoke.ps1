. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -15 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 6
for($n=0;$n -lt 3 -and (Get-Addon).nativeTopMenu -eq 1001;$n++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 500}
Send-Game @('set GameHour to 12','tgm','player.SetPos X -72500','player.SetPos Y 2000')|Out-Null
Start-Sleep -Seconds 2
Park-Hands
Send-Game @('player.AddItem 0000434F 1','sc_weapons')|Out-Null;Start-Sleep -Milliseconds 250
Send-Game @('sc_pick 0000434F')|Out-Null;Start-Sleep -Seconds 1
Send-XR @('hand r grip pose .22 .95 -.43 0 55 0','hand l grip pose -.3 .75 -.23 0 55 0')
for($n=0;$n -lt 3;$n++){Send-XR @('btn menu press 80');Start-Sleep -Milliseconds 160}
Start-Sleep -Seconds 1
$a=Get-Addon

Save-Eyes 'tutorial-visual-bounds'
Send-XR @('head to .04 1.1 0 10 -20 0 1000','hand r to grip .1 .90 -.50 -10 60 0 1200')
Start-Sleep -Milliseconds 1500
Save-Eyes 'tutorial-visual-bounds-moved'
