. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -55 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('sc_trace_impacts','player.AddItem 0000434F 1','player.AddItem 00004241 60','player.SetAngle X 0','player.SetWeaponOut 0','set GameHour to 12','sc_weapons')|Out-Null
Start-Sleep -Milliseconds 300
Send-Game @('sc_pick 0000434F')|Out-Null;Start-Sleep -Milliseconds 1000
Send-XR @('hand r grip pose .20 .90 -.50 0 -5 0','hand l grip pose -.3 .7 -.2 0 0 0')
Send-Game @('player.SetCurrentAmmoRounds 12')|Out-Null
for($i=0;$i -lt 4;$i++){Send-XR @('trigger r pull 180');Start-Sleep -Milliseconds 700}
Get-Addon|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\work\vehicle-controls\native-bullet-state.json"
Park-Hands
Send-XR @('head pose 0 1.1 0 0 -75 0')
Start-Sleep -Milliseconds 400
Save-Eyes 'native-bullet-reference'
Send-Game @('sc_glass_bullets','PlaySound FXBulletImpactGlass')|Out-Null
Send-XR @('hand r grip pose -.15 .90 -.60 -45 -5 0');Start-Sleep -Milliseconds 200
Send-XR @('trigger r pull 180');Start-Sleep -Milliseconds 700
Park-Hands
Send-XR @('head pose 0 1.1 0 -20 -75 0')
Start-Sleep -Milliseconds 500
Save-Eyes 'native-glass-texture-reference'
Start-Sleep -Seconds 3

