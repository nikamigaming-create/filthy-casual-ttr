. "$PSScriptRoot\runtime-helpers.ps1"
$film="$script:Repo\outputs\bighorner";New-Item -ItemType Directory -Force $film|Out-Null
Remove-Item "$script:Session\bighorner-state.json" -ErrorAction SilentlyContinue
Send-XR @('capture off','pace free 60','input clear','profile touch','head pose 0 1.1 0 0 -15 0','btn a press 180')
Send-Game @('load FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('tgm','tcai','player.SetWeaponOut 0','set GameHour to 12','player.SetPosEx -56000 -79000 6000')|Out-Null
Start-Sleep -Seconds 3
$n=Get-Native 'player.PlaceAtMe VCrFarmBigHorner 1';$id=('{0:X8}' -f [int32]$n)
$px=Get-Native 'player.GetPos X';$py=(Get-Native 'player.GetPos Y')+100;$pz=(Get-Native 'player.GetPos Z')+5
Send-Game @("$id.SetPosEx $px $py $pz","$id.SetAngle Z 0","$id.SetRestrained 1")|Out-Null;Start-Sleep -Seconds 2
Send-Game @("player.MoveTo $id 85 0 0","player.SetAngle Z 270")|Out-Null;Start-Sleep -Seconds 1
function Ride-State {Get-Content "$script:Session\bighorner-state.json" -Raw|ConvertFrom-Json}
if(!(Test-Path "$script:Session\bighorner-state.json")){throw 'Bighorner not found by native mode'}
$r=Ride-State;$r|ConvertTo-Json|Set-Content "$film\before.json"
$p=$r.back;$yaw=[Math]::Atan2(-$p[0],-$p[2])*180/[Math]::PI;$pitch=[Math]::Atan2($p[1]-1.65,[Math]::Sqrt($p[0]*$p[0]+$p[2]*$p[2]))*180/[Math]::PI
Send-XR @(("head to 0 1.1 0 {0:F4} {1:F4} 0 1000" -f $yaw,$pitch));Start-Sleep -Milliseconds 1200;Save-Eyes 'bighorner-before'

function Snapshot([string]$Name){$a=Get-Addon;$r=Ride-State;$ss=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;@{animal=$r;head=$ss.head;hands=@($ss.handL,$ss.handR);grips=$a.nativeGripPoints;tracked=$a.nativeGripTracking;contact=(Get-Content "$script:Session\bighorner-contact.json" -Raw|ConvertFrom-Json)}|ConvertTo-Json -Depth 12|Set-Content "$film\probe-$Name.json"}
Snapshot 'start'
Start-Sleep -Seconds 4;Snapshot 'wait'
$r=Ride-State;$a=Get-Addon;$ss=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
foreach($h in 0,1){$hand=if($h){'r'}else{'l'};$hp=if($h){$ss.handR.pos}else{$ss.handL.pos};$target=@($r.back);$distance=[Math]::Sqrt($target[0]*$target[0]+$target[2]*$target[2]);$target[0]-=.48*$target[0]/$distance;$target[2]-=.48*$target[2]/$distance;$q=@();for($j=0;$j -lt 3;$j++){$q+=($hp[$j]+$target[$j]-$a.nativeGripTracking[$h][$j])};$q[0]+=if($h){.09}else{-.09};$q[1]+=.12;Send-XR @(("hand {0} to grip {1:F5} {2:F5} {3:F5} {4:F4} 35 0 1200" -f $hand,$q[0],$q[1],$q[2],$yaw))}
Start-Sleep -Seconds 2;Snapshot 'reach';Save-Eyes 'bighorner-reach-diagnostic'
Send-XR @('grip l 1','grip r 1');Start-Sleep -Milliseconds 250;Snapshot 'squeeze';Start-Sleep -Seconds 1;Snapshot 'held'
