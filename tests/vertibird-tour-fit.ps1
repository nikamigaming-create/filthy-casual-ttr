. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 8 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 7
for($i=0;$i -lt 4 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('set GameHour to 12','player.SetPos X -72500','player.SetPos Y 2000','player.SetAngle Z 0')|Out-Null
Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 2'|Out-Null
Start-Sleep -Seconds 10
$bx=Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 4';$by=Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 5';$bz=Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 6'
Write-Output "BIRD CENTER $bx $by $bz"
Save-Eyes 'vertibird-tour-approach'
$px=Get-Native 'player.GetPos X';$py=Get-Native 'player.GetPos Y'
Send-XR @('stick l 0 .45');Start-Sleep -Seconds 3;Send-XR @('stick l 0 0')
$x=Get-Native 'player.GetPos X';$y=Get-Native 'player.GetPos Y'
Write-Output "WALK DELTA $($x-$px) $($y-$py)"
Save-Eyes 'vertibird-tour-near'
# Native boarding action, followed by original pilot chair prompt.
Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 3'|Out-Null;Start-Sleep -Seconds 2
Save-Eyes 'vertibird-tour-board-prompt'
Send-XR @('btn a press 180');Start-Sleep -Seconds 6
Save-Eyes 'vertibird-tour-inside'
Get-Native 'Call (CompileScript "FCVertibirdTest.txt") 0'|Out-Null
Get-Native 'Call (CompileScript "FCVertibirdTest.txt") 1'|Out-Null;Start-Sleep -Seconds 2;Send-XR @('btn a press 180');Start-Sleep -Seconds 12
Send-XR @('head pose 0 1.1 0 0 -5 0')
Save-Eyes 'vertibird-tour-seat-revised'
Send-XR @('btn a down');Start-Sleep -Seconds 2;Send-XR @('btn a up','stick l 0 .5');Start-Sleep -Seconds 3;Send-XR @('stick l 0 0')
Save-Eyes 'vertibird-tour-seat-moving'
Send-XR @('btn b press 350');Start-Sleep -Seconds 18
Assert-Equal (Get-Native 'GetVariable "Piloting" (GetFormFromMod "Player Vertibird.esm" "000AE7")') 0 'Native landing completes'
Save-Eyes 'vertibird-tour-afterland'
