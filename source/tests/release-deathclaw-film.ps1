. "$PSScriptRoot\runtime-helpers.ps1"
$out="$script:Repo\outputs\deathclaw";New-Item -ItemType Directory -Force $out|Out-Null
Send-XR @('capture off','pace free 60','input clear','profile touch','head pose 0 1.1 0 0 -30 0','btn a press 180')
Send-Game @('load FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('player.SetWeaponOut 0','set GameHour to 12','player.SetPosEx -56000 -79000 6000','tdetect')|Out-Null
Start-Sleep -Seconds 3
$n=Get-Native 'player.PlaceAtMe VCrTier4DeathClawMed 1';$id=('{0:X8}' -f [int32]$n)
$px=Get-Native 'player.GetPos X';$py=(Get-Native 'player.GetPos Y')+100;$pz=(Get-Native 'player.GetPos Z')+5
Send-Game @("$id.SetPosEx $px $py $pz","$id.SetAngle Z 0","$id.SetRestrained 1","player.MoveTo $id 0 -75 0","player.SetAngle Z 0")|Out-Null;Start-Sleep -Seconds 2
function Read-Ride {for($read=0;$read -lt 20;$read++){try{$v=Get-Content "$script:Session\bighorner-state.json" -Raw|ConvertFrom-Json;if($null -ne $v -and $v.PSObject.Properties.Name -contains "active"){return $v}}catch{};Start-Sleep -Milliseconds 10};throw "Ride diagnostic snapshot unavailable"}
function Touch-Back([switch]$ReleaseStaging) {
 $r=Read-Ride;$a=Get-Addon;$ss=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 foreach($h in 0,1){$hand=if($h){'r'}else{'l'};$p=if($h){$ss.handR.pos}else{$ss.handL.pos};$target=@($r.back);$d=[Math]::Sqrt($target[0]*$target[0]+$target[2]*$target[2]);$target[0]-=.35*$target[0]/$d;$target[2]-=.35*$target[2]/$d;$q=@();for($j=0;$j -lt 3;$j++){$q+=($p[$j]+$target[$j]-$a.nativeGripTracking[$h][$j])};$q[0]+=if($h){.18}else{-.18}
  Send-XR @(("hand {0} to grip {1:F5} {2:F5} {3:F5} 0 45 0 800" -f $hand,$q[0],$q[1],$q[2]))}
 Start-Sleep -Milliseconds 950;if($ReleaseStaging){Send-Game @("$id.SetRestrained 0")|Out-Null};Send-XR @('grip l 1','grip r 1');Start-Sleep -Milliseconds 750
 if($ReleaseStaging){$deadline=[DateTime]::UtcNow.AddSeconds(3);do{Start-Sleep -Milliseconds 100;$r=Read-Ride}while((!$r.active -or $r.seconds -lt .1) -and [DateTime]::UtcNow -lt $deadline)}
}
Touch-Back
Assert-Equal (Read-Ride).active $false 'Standing behind cannot mount Deathclaw'
Park-Hands
Send-XR @('stick r 0 -1');Start-Sleep -Milliseconds 200;Send-XR @('stick r center');Start-Sleep -Milliseconds 600
Assert-Equal (Get-Native 'player.IsSneaking') 1 'Native player is sneaking'
Send-Game @("player.MoveTo $id 0 85 0","player.SetAngle Z 180")|Out-Null;Start-Sleep -Seconds 1
Touch-Back
Assert-Equal (Read-Ride).active $false 'Sneaking in front cannot mount Deathclaw'
Park-Hands
Send-Game @("player.MoveTo $id 0 -75 0","player.SetAngle Z 0")|Out-Null;Start-Sleep -Seconds 1
# Force the native detection result only inside this isolated negative fixture.
Send-Game @('tdetect','player.ForceActorDetectionValue 1000 0',"$id.ForceActorDetectionValue 1000 0")|Out-Null;Start-Sleep -Seconds 2
Touch-Back
$denied=Get-Content "$script:Session\deathclaw-eligibility.json" -Raw|ConvertFrom-Json
Assert-Equal $denied.detected 1 'Native detected state blocks rear mounting'
Assert-Equal (Read-Ride).active $false 'Both grips cannot bypass detection'
Park-Hands
Send-Game @('player.ForceActorDetectionValue 0',"$id.ForceActorDetectionValue 0","$id.StopCombat",'tdetect')|Out-Null;Start-Sleep -Seconds 1

# Start a fresh encounter after the spotted-state refusal fixture. Toggling
# detection off intentionally preserves the old actor's awareness memory.
Send-Game @("$id.SetPosEx -54500 -77500 5350")|Out-Null
$n=Get-Native 'player.PlaceAtMe VCrTier4DeathClawMed 1';$id=('{0:X8}' -f [int32]$n)
Send-Game @("$id.SetPosEx $px $py $pz","$id.SetAngle Z 0","$id.SetRestrained 1","player.MoveTo $id 0 -75 0","player.SetAngle Z 0")|Out-Null;Start-Sleep -Seconds 2
$eligible=Get-Content "$script:Session\deathclaw-eligibility.json" -Raw|ConvertFrom-Json
Assert-Equal $eligible.eligible $true 'Native hidden rear approach is eligible'
Send-XR @('video on');$state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
$stop="$out\ride.stop";Remove-Item $stop -ErrorAction SilentlyContinue
$rec=Start-Process python.exe -ArgumentList @("$PSScriptRoot\record-eyes.py",'--pid',$state.pid,'--out',"$out\final-stereo.mp4",'--seconds',180,'--fps',30,'--audio','--stop-file',$stop) -WindowStyle Hidden -PassThru
$timeline=[Diagnostics.Stopwatch]::StartNew();$events=[Collections.Generic.List[object]]::new()
function Event([string]$Name){$events.Add(@{event=$Name;seconds=$timeline.Elapsed.TotalSeconds})}
try{
 Start-Sleep -Seconds 1;Touch-Back -ReleaseStaging
 Assert-Equal (Read-Ride).active $true 'Both grips mount hidden rear Deathclaw';Event 'mounted'
 Send-XR @('grip l 0','grip r 0','head to 0 1.1 0 0 -35 0 800')
 foreach($h in 0,1){$hand=if($h){'r'}else{'l'};$r=Read-Ride;$a=Get-Addon;$ss=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$p=if($h){$ss.handR.pos}else{$ss.handL.pos};if(!$h){$leftOffset=@();for($j=0;$j -lt 3;$j++){$leftOffset+=($a.nativeGripTracking[0][$j]-$p[$j])}};$q=@();for($j=0;$j -lt 3;$j++){$q+=($p[$j]+$r.sockets[$h][$j]-$a.nativeGripTracking[$h][$j])};Send-XR @(("hand {0} to grip {1:F5} {2:F5} {3:F5} 0 45 0 800" -f $hand,$q[0],$q[1],$q[2]))}
 Start-Sleep -Milliseconds 900;Send-XR @('grip l 1','grip r 1');Start-Sleep -Milliseconds 500
 $r=Read-Ride;Assert-Equal (($r.held[0] -eq 1)-and($r.held[1] -eq 1)) $true 'Both hands physically hold Deathclaw body'
 Save-Eyes 'deathclaw-holding-on';Event 'holding'
 Send-XR @('grip r 0','hand r to grip .35 1.25 -.3 0 -10 0 1200')
 $lastHeld=[DateTime]::UtcNow;$deadline=[DateTime]::UtcNow.AddSeconds(65)
 do{
  Start-Sleep -Milliseconds 60;$r=Read-Ride
  $p=@();for($j=0;$j -lt 3;$j++){$p+=($r.sockets[0][$j]-$leftOffset[$j])};Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} 0 45 0 100" -f $p[0],$p[1],$p[2]))
  if(!$r.active){throw 'Lost Deathclaw mount'}
  if($r.held[0] -eq 1){$lastHeld=[DateTime]::UtcNow}elseif(([DateTime]::UtcNow-$lastHeld).TotalSeconds -gt .5){throw 'Lost actual Deathclaw body hold'}
  if($r.seconds -lt 59.9 -and $r.controlled){throw 'Deathclaw granted steering before one minute'}
 }while(!$r.controlled -and [DateTime]::UtcNow -lt $deadline)
 Assert-Equal $r.controlled $true 'Full one-minute Deathclaw challenge earns steering';Event 'earned'
 $r|ConvertTo-Json -Depth 6|Set-Content "$out\earned-control.json";Save-Eyes 'deathclaw-earned-control'
 $before=@($r.position);Send-XR @('stick l 0 .65','stick r .25 0');Start-Sleep -Seconds 3;Send-XR @('stick l center','stick r center');$r=Read-Ride
 Assert-Equal ([Math]::Sqrt([Math]::Pow($before[0]-$r.position[0],2)+[Math]::Pow($before[1]-$r.position[1],2)) -gt 15) $true 'Earned sticks move Deathclaw'
 Park-Hands;Send-Game @('tdetect','ToggleImmortalMode 0','player.SetAV Health 40')|Out-Null;Send-XR @('btn b press 700');Start-Sleep -Seconds 2
 Assert-Equal (Read-Ride).active $false 'B dismount ends Deathclaw ride';Event 'dismounted'
 Assert-Equal (Get-Native 'GetPlayerControlsDisabled 1') 0 'Deathclaw dismount restores movement'
 $q=[char]34;Assert-Equal (Get-Native ("Call (CompileScript {0}FCRideQuery.txt{0}) {0}{1}{0} 0" -f $q,$id)) 1 'Dismounted Deathclaw is natively hostile'
 $r=Read-Ride;$p=$r.front;$yaw=[Math]::Atan2(-$p[0],-$p[2])*180/[Math]::PI;$pitch=[Math]::Atan2($p[1]-1.65,[Math]::Sqrt($p[0]*$p[0]+$p[2]*$p[2]))*180/[Math]::PI
 Send-XR @(("head to 0 1.1 0 {0:F3} {1:F3} 0 700" -f $yaw,$pitch),'hand l to grip -.28 1.05 -.4 0 0 0 800','hand r to grip .28 1.05 -.4 0 0 0 800')
 Save-Eyes 'deathclaw-hostile-dismount';Event 'hostile'
 $deadline=[DateTime]::UtcNow.AddSeconds(25)
 do{Start-Sleep -Milliseconds 500;$dead=Get-Native 'player.GetDead'}while(!$dead -and [DateTime]::UtcNow -lt $deadline)
 Assert-Equal $dead 1 'Native Deathclaw kills the dismounted player';Event 'dead'
 Save-Eyes 'deathclaw-showcase-death';Start-Sleep -Seconds 3

}finally{$events|ConvertTo-Json|Set-Content "$out\final-events.json";Set-Content $stop stop;if(!$rec.WaitForExit(12000)){throw 'Recorder did not finish'};Send-XR @('video off')}
