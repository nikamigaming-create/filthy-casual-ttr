. "$PSScriptRoot\runtime-helpers.ps1"
$film="$script:Repo\outputs\bighorner";New-Item -ItemType Directory -Force $film|Out-Null
Remove-Item "$script:Session\bighorner-state.json" -ErrorAction SilentlyContinue
Send-XR @('capture off','pace free 60','input clear','profile touch','head pose 0 1.1 0 0 -15 0','btn a press 180')
Send-Game @('load FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('tgm','tcai','player.SetWeaponOut 0','set GameHour to 12','player.SetPos X -54170','player.SetPos Y -78170','player.SetPos Z 5400')|Out-Null
Start-Sleep -Seconds 3
$n=Get-Native 'player.PlaceAtMe VCrFarmBigHorner 1';$id=('{0:X8}' -f [int32]$n)
Send-Game @("$id.SetPosEx -54170 -78070 5330","$id.SetAngle Z 0","$id.SetRestrained 1")|Out-Null;Start-Sleep -Seconds 2
Send-Game @("player.MoveTo $id 85 0 0","player.SetAngle Z 270")|Out-Null;Start-Sleep -Seconds 1
function Ride-State {Get-Content "$script:Session\bighorner-state.json" -Raw|ConvertFrom-Json}
if(!(Test-Path "$script:Session\bighorner-state.json")){throw 'Bighorner not found by native mode'}
$r=Ride-State;$r|ConvertTo-Json|Set-Content "$film\before.json"
$p=$r.back;$yaw=[Math]::Atan2(-$p[0],-$p[2])*180/[Math]::PI;$pitch=[Math]::Atan2($p[1]-1.65,[Math]::Sqrt($p[0]*$p[0]+$p[2]*$p[2]))*180/[Math]::PI
Send-XR @(("head to 0 1.1 0 {0:F4} {1:F4} 0 1000" -f $yaw,$pitch));Start-Sleep -Milliseconds 1200;Save-Eyes 'bighorner-before'
Send-XR @('video on');$state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
$stop="$film\ride.stop";Remove-Item $stop,"$film\ride-stereo.ready.json" -ErrorAction SilentlyContinue
$rec=Start-Process python.exe -ArgumentList @("$PSScriptRoot\record-eyes.py",'--pid',$state.pid,'--out',"$film\ride-stereo.mp4",'--seconds',70,'--fps',30,'--audio','--stop-file',$stop) -WindowStyle Hidden -PassThru
try{
 Start-Sleep -Seconds 2
 # Stage beside the collision hull, then touch the near flank rather than
 # reaching through the animal to the center of its spine.
 $r=Ride-State;$horizontal=[Math]::Sqrt($r.back[0]*$r.back[0]+$r.back[2]*$r.back[2])
 Assert-Equal ($horizontal -lt 1.40) $true 'Standing beside the reachable flank'
 Start-Sleep -Milliseconds 500
 for($attempt=0;$attempt -lt 4 -and !(Ride-State).active;$attempt++){
  $r=Ride-State;$a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  foreach($h in 0,1){$hand=if($h){'r'}else{'l'};$p=if($h){$s.handR.pos}else{$s.handL.pos};$target=@($r.back);$distance=[Math]::Sqrt($target[0]*$target[0]+$target[2]*$target[2]);$target[0]-=.48*$target[0]/$distance;$target[2]-=.48*$target[2]/$distance;$q=@();for($j=0;$j -lt 3;$j++){$q+=($p[$j]+$target[$j]-$a.nativeGripTracking[$h][$j])};$q[0]+=if($h){.09}else{-.09};$q[1]+=.12
   Send-XR @(("hand {0} to grip {1:F5} {2:F5} {3:F5} {4:F4} 35 0 1200" -f $hand,$q[0],$q[1],$q[2],$yaw))
  }
  Start-Sleep -Milliseconds 1350;Send-XR @('grip l 1','grip r 1');Start-Sleep -Milliseconds 1100
 }
 Assert-Equal (Ride-State).active $true 'Both grips mount Bighorner'
 Send-Game @("$id.SetRestrained 0")|Out-Null
 function Ride-Head([double]$Turn,[double]$Pitch,[int]$Ms=1400){Send-XR @(("head to 0 1.1 0 {0:F4} {1:F4} 0 {2}" -f ($yaw+$Turn),$Pitch,$Ms))}
 function Ride-Hand([string]$Hand,[double]$X,[double]$Y,[double]$Z,[double]$Pitch=15,[int]$Ms=1300){
  $theta=$yaw*[Math]::PI/180;$xx=[Math]::Cos($theta)*$X+[Math]::Sin($theta)*$Z;$zz=-[Math]::Sin($theta)*$X+[Math]::Cos($theta)*$Z
  Send-XR @(("hand {0} to grip {1:F5} {2:F5} {3:F5} {4:F4} {5:F4} 0 {6}" -f $Hand,$xx,$Y,$zz,$yaw,$Pitch,$Ms))
 }
 Send-XR @('grip l 0','grip r 0');Ride-Head 0 -32;Ride-Hand l -.3 .9 -.35;Ride-Hand r .3 .9 -.35
 Start-Sleep -Seconds 3;Assert-Equal (Ride-State).active $true 'Mounted after hand release';Save-Eyes 'bighorner-mounted'
 Start-Sleep -Seconds 3
 Ride-Hand r .40 1.3 -.35 -10;Ride-Head 12 -25;Start-Sleep -Seconds 3
 Ride-Hand l -.40 1.3 -.35 -10;Ride-Hand r .3 .85 -.35;Ride-Head -12 -25;Start-Sleep -Seconds 3
 Save-Eyes 'bighorner-free-hands';Ride-Head 0 -45
 foreach($h in 0,1){$hand=if($h){'r'}else{'l'};$r=Ride-State;$a=Get-Addon;$ss=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$hp=if($h){$ss.handR.pos}else{$ss.handL.pos};$q=@();for($j=0;$j -lt 3;$j++){$q+=($hp[$j]+$r.sockets[$h][$j]-$a.nativeGripTracking[$h][$j])};Send-XR @(("hand {0} to grip {1:F5} {2:F5} {3:F5} {4:F4} 45 0 1300" -f $hand,$q[0],$q[1],$q[2],$yaw))}
 Start-Sleep -Milliseconds 1500;Send-XR @('grip l 1','grip r 1');Start-Sleep -Seconds 2
 $contact=Ride-State;Assert-Equal (($contact.held[0] -eq 1) -and ($contact.held[1] -eq 1)) $true 'Both hands hold on at the withers'
 Save-Eyes 'bighorner-holding-on';Start-Sleep -Seconds 3
 Send-XR @('grip r 0');Ride-Hand r .4 1.3 -.35 -10;Start-Sleep -Seconds 3
 Send-XR @('grip l 0');Ride-Hand l -.3 .85 -.35;Ride-Head 0 -28;Start-Sleep -Seconds 2
 Send-XR @('btn b press 180');Start-Sleep -Seconds 2
 Assert-Equal (Get-Native 'GetPlayerControlsDisabled 1') 0 'Dismount restores movement'
 Save-Eyes 'bighorner-dismounted';Start-Sleep -Seconds 3
}finally{
 Set-Content $stop stop;if(!$rec.WaitForExit(12000)){throw 'Recorder did not finish'};Send-XR @('video off')
 Copy-Item 'D:\SteamLibrary\steamapps\common\Fallout New Vegas\FilthyCasualVehicles.log' "$film\vehicle.log" -Force
}
