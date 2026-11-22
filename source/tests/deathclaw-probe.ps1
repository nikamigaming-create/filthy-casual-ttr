. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','profile touch','head pose 0 1.1 0 0 -15 0','btn a press 180')
Send-Game @('load FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('tgm','player.SetWeaponOut 0','set GameHour to 12','player.SetPosEx -56000 -79000 6000','tdetect')|Out-Null
Start-Sleep -Seconds 3
$n=Get-Native 'player.PlaceAtMe VCrTier4DeathClawMed 1';$id=('{0:X8}' -f [int32]$n)
$px=Get-Native 'player.GetPos X';$py=(Get-Native 'player.GetPos Y')+100;$pz=(Get-Native 'player.GetPos Z')+5
Send-Game @("$id.SetPosEx $px $py $pz","$id.SetAngle Z 0","$id.SetRestrained 1","player.MoveTo $id 0 -75 0","player.SetAngle Z 0")|Out-Null;Start-Sleep -Seconds 2
function Read-Ride {Get-Content "$script:Session\bighorner-state.json" -Raw|ConvertFrom-Json}
Read-Ride|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\work\deathclaw\probe-before.json"
Send-XR @('stick r 0 -1');Start-Sleep -Milliseconds 200;Send-XR @('stick r center');Start-Sleep -Milliseconds 800
Write-Output "SNEAK $(Get-Native 'player.IsSneaking')"
$r=Read-Ride;$a=Get-Addon;$ss=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
foreach($h in 0,1){$hand=if($h){'r'}else{'l'};$p=if($h){$ss.handR.pos}else{$ss.handL.pos};$target=@($r.back);$target[2]+=.35;$q=@();for($j=0;$j -lt 3;$j++){$q+=($p[$j]+$target[$j]-$a.nativeGripTracking[$h][$j])};$q[0]+=if($h){.18}else{-.18}
Send-XR @(("hand {0} to grip {1:F5} {2:F5} {3:F5} 0 45 0 1000" -f $hand,$q[0],$q[1],$q[2]))}
Start-Sleep -Milliseconds 1200;Send-XR @('grip l 1','grip r 1');Start-Sleep -Seconds 2
Read-Ride|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\work\deathclaw\probe-mounted.json"
Save-Eyes 'deathclaw-mount-probe'
Get-Content "$script:Session\bighorner-contact.json"
if((Read-Ride).active){Send-XR @('grip l 0','grip r 0','btn b press 180');Start-Sleep -Seconds 1}

