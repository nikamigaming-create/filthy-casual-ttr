. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','profile touch','head pose 0 1.1 0 0 -60 0','btn a press 180')
Send-Game @('load FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('tgm','tai','player.SetWeaponOut 0','set GameHour to 12','player.SetPos X -54170','player.SetPos Y -78170','player.SetPos Z 5400')|Out-Null
Start-Sleep -Seconds 3
$fn='Call (CompileScript "FCBottleImpact.txt") '
function Capture-Impact([string]$Name,[float]$X,[float]$Y,[float]$Height=220){
 $before=(Get-Addon).bottleBreaks
 $lineCount=@(Get-Content $script:GameLog).Count
 $id=('{0:X8}' -f [int32](Get-Native ($fn+('"" 0 {0} 0 {1} {2}' -f $Height,$X,$Y))))
 Send-Game @("sc_track_bottle $id")|Out-Null;Start-Sleep -Milliseconds 1800
 if($Height -lt 20){Assert-Equal (Get-Addon).bottleBreaks $before 'Gentle impact remains intact';return}
 Assert-Equal (Get-Addon).bottleBreaks ($before+1) "$Name native impact shatters once"
 $events=@(Get-Content $script:GameLog|Select-Object -Skip $lineCount)
 $events|Set-Content "$script:Repo\work\food-glass-20261113\surface-$Name.log"
 Assert-Equal (@($events|Where-Object {$_ -match 'GLASS NATIVE.*init=1.*verts=[1-9].*tris=[1-9]'}).Count -gt 0) $true "$Name produces actual projected native geometry"
 Send-Game @('sc_glass_state')|Out-Null;$g=@(Get-Content "$script:Session\glass-state.json" -Raw|ConvertFrom-Json)[0]
 # Observe the native impact from above; account for the saved +0.55 seated offset.
 $dx=$g.tracking[0];$dy=$g.tracking[1]-1.65;$dz=$g.tracking[2]
 $yaw=[Math]::Atan2(-$dx,-$dz)*180/[Math]::PI
 $pitch=[Math]::Atan2($dy,[Math]::Sqrt($dx*$dx+$dz*$dz))*180/[Math]::PI
 Send-XR @(("head pose 0 1.1 0 {0:F3} {1:F3} 0" -f $yaw,$pitch));Start-Sleep -Milliseconds 250
 Save-Eyes "surface-$Name-2s"
 
}
Capture-Impact 'gentle-desert' 0 80 8
Capture-Impact 'desert' 20 80
# Terrain-slope acceptance requires a nonvertical hit normal; no flat-ground claim.
foreach($case in @(@('concrete','000762A7'),@('wood','0014DA83'),@('metal','0009B590'),@('rock','0012FC68'))){
 $n=Get-Native ('Call (CompileScript "FCGlassSurface.txt") '+[char]34+$case[1]+[char]34+' 0 350 0')
 Assert-Equal ($n -ne 0) $true "Actual surface reference spawned"
 $prop=('{0:X8}' -f [int32]$n);Start-Sleep -Seconds 1
 Capture-Impact $case[0] 0 350
 Send-Game @("$prop.Disable","$prop.MarkForDelete")|Out-Null;Start-Sleep -Milliseconds 350
}
# Existing real road mesh at the second map location, not a staged plane.
Send-Game @('player.SetPos X -72500','player.SetPos Y 2000','player.SetPos Z 8500')|Out-Null;Start-Sleep -Seconds 3
Capture-Impact 'road' 0 80
