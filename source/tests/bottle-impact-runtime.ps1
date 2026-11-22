. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -55 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('player.SetWeaponOut 0','set GameHour to 12','player.SetPos X -72500','player.SetPos Y 2000','player.SetAngle Z 0')|Out-Null
Start-Sleep -Seconds 2
$fn='Call (CompileScript "FCBottleImpact.txt") '
$a=Get-Addon
$rx=$a.nativeGripPoints[1][0]-$a.nativeGripPoints[0][0];$ry=$a.nativeGripPoints[1][1]-$a.nativeGripPoints[0][1]
$len=[Math]::Sqrt($rx*$rx+$ry*$ry);$rx/=$len;$ry/=$len
$offset=(' {0:F3} {1:F3}' -f (-$ry*100+$rx*20),($rx*100+$ry*20))
$before=$a.bottleBreaks
$soft=Get-Native ($fn+'"" 0 8 0'+$offset)
Assert-Equal ($soft -ne 0) $true 'Native soft bottle fixture spawned'
$id=('{0:X8}' -f [int32]$soft)
Send-Game @("sc_track_bottle $id")|Out-Null
Start-Sleep -Seconds 2
Assert-Equal (Get-Addon).bottleBreaks $before 'Gentle floor drop keeps bottle intact'
Assert-Equal (Get-Native ($fn+'"'+$id+'" 2 0 0 0 0')) 1 'Soft fixture removed natively'
Send-XR @('head pose 0 1.1 0 0 -55 0')
Save-Eyes 'bottle-ground-before'
$audio=Start-Process -FilePath (Get-Command python).Source -ArgumentList @('-u',"$script:Repo\work\vehicle-controls\record-bottle-audio.py") -PassThru -WindowStyle Hidden
$limit=[DateTime]::UtcNow.AddSeconds(8)
while(!(Test-Path "$script:Repo\work\vehicle-controls\audio-ready.txt")){if([DateTime]::UtcNow -gt $limit){throw 'Audio capture did not start'};Start-Sleep -Milliseconds 50}
$hard=Get-Native ($fn+'"" 0 220 0'+$offset)
Assert-Equal ($hard -ne 0) $true 'Native hard bottle fixture spawned'
$id=('{0:X8}' -f [int32]$hard)
Send-Game @("sc_track_bottle $id")|Out-Null

Start-Sleep -Seconds 2
$a=Get-Addon
$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\work\vehicle-controls\bottle-impact-result.json"
Send-Game @('sc_glass_state')|Out-Null
$glass=@(Get-Content "$script:Session\glass-state.json" -Raw|ConvertFrom-Json)[0]
$glass|ConvertTo-Json -Compress|Write-Output
$dx=$glass.tracking[0];$dy=$glass.tracking[1]-1.1;$dz=$glass.tracking[2]
$yaw=[Math]::Atan2(-$dx,-$dz)*180/[Math]::PI
$pitch=[Math]::Atan2($dy,[Math]::Sqrt($dx*$dx+$dz*$dz))*180/[Math]::PI
Send-XR @(("head pose 0 1.1 0 {0:F3} {1:F3} 0" -f $yaw,$pitch));Start-Sleep -Milliseconds 250
Save-Eyes 'bottle-impact-glass'
Assert-Equal $a.bottleBreaks ($before+1) 'Hard native collision shatters the bottle exactly once'
# Manager list is a pending queue, not the active rendered decal count.
Start-Sleep -Seconds 11
Save-Eyes 'bottle-impact-after12s'
# Native pending queue count cannot establish the rendered lifetime.
