param([switch]$SignsOnly,[switch]$SkipSoft,[switch]$GroundOnly)
. "$PSScriptRoot\runtime-helpers.ps1"
$script:Film="$script:Repo\outputs\bottle-demo"
New-Item -ItemType Directory -Force $script:Film|Out-Null
$fn='Call (CompileScript "FCBottleImpact.txt") '
function Target([string]$Ref){Send-Game @("sc_target $Ref")|Out-Null;return Get-Content "$script:Session\target-probe.json" -Raw|ConvertFrom-Json}
function Aim([double[]]$P,[double]$HeadY=1.1,[int]$Ms=1000){
 $yaw=[Math]::Atan2(-$P[0],-$P[2])*180/[Math]::PI;$pitch=[Math]::Atan2(($P[1]-$HeadY-.55),[Math]::Sqrt($P[0]*$P[0]+$P[2]*$P[2]))*180/[Math]::PI
 Send-XR @(("head to 0 {0:F4} 0 {1:F4} {2:F4} 0 {3}" -f $HeadY,$yaw,$pitch,$Ms));Start-Sleep -Milliseconds ($Ms+100)
}
function Grab-Bottle([string]$Ref){
 Send-XR @('grip r 0');Start-Sleep -Milliseconds 150
 for($i=0;$i -lt 5;$i++){
  $a=Get-Addon;if($a.nativeGrabRef -eq [Convert]::ToUInt32($Ref,16)){return}
  $t=Target $Ref;if(!$t.ok){throw 'Bottle target missing'}
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $p=@();for($j=0;$j -lt 3;$j++){$p+=($s.handR.pos[$j]+$t.tracking[$j]-$a.nativeGripTracking[1][$j])};$p[1]+=.025
  Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 0 55 4 900" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1050
  Send-XR @('grip r 1');Start-Sleep -Milliseconds 250
  if((Get-Addon).nativeGrabRef -eq [Convert]::ToUInt32($Ref,16)){return};Send-XR @('grip r 0')
 }
 throw "Native pickup failed $Ref"
}
function Move-Bottle([string]$Ref,[double[]]$P,[int]$Ms=1200){
 $t=Target $Ref;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $q=@();for($j=0;$j -lt 3;$j++){$q+=($s.handR.pos[$j]+$P[$j]-$t.tracking[$j])}
 Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 0 55 4 {3}" -f $q[0],$q[1],$q[2],$Ms));Start-Sleep -Milliseconds ($Ms+150)
}
function New-Bottle{
 $n=Get-Native ($fn+'"" 0 5 0'+$script:PickupOffset);$id=('{0:X8}' -f [int32]$n)
 Start-Sleep -Milliseconds 700;Send-Game @("sc_track_bottle $id")|Out-Null;return $id
}
function Begin-Clip([string]$Name){
 $script:Clip=$Name;$script:Stop="$script:Film\$Name.stop";$ready="$script:Film\$Name-stereo.ready.json"
 foreach($p in @($script:Stop,$ready)){if(Test-Path -LiteralPath $p){Remove-Item -LiteralPath $p}}
 Send-XR @('video on');$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $script:Recorder=Start-Process python.exe -ArgumentList @("$PSScriptRoot\record-eyes.py",'--pid',$s.pid,'--out',"$script:Film\$Name-stereo.mp4",'--seconds',70,'--fps',30,'--audio','--stop-file',$script:Stop) -WindowStyle Hidden -PassThru
 $deadline=[DateTime]::UtcNow.AddSeconds(12);while(!(Test-Path $ready)){if([DateTime]::UtcNow -gt $deadline){throw 'Recorder failed to start'};Start-Sleep -Milliseconds 50}
 $script:ClipClock=[Diagnostics.Stopwatch]::StartNew();$script:Events=[Collections.Generic.List[object]]::new();Start-Sleep -Milliseconds 1200
}
function Event([string]$What){$script:Events.Add(@{event=$What;time=$script:ClipClock.Elapsed.TotalSeconds})}
function End-Clip{
 Start-Sleep -Seconds 3;Set-Content -LiteralPath $script:Stop -Value stop
 if(!$script:Recorder.WaitForExit(12000)){throw 'Recorder did not finish'}
 $script:Events|ConvertTo-Json|Set-Content "$script:Film\$script:Clip-events.json"
 Send-XR @('video off')
}
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -30 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('player.SetWeaponOut 0','set GameHour to 12','player.SetPos X -72500','player.SetPos Y 2000')|Out-Null;Start-Sleep -Seconds 2
$a=Get-Addon;$rx=$a.nativeGripPoints[1][0]-$a.nativeGripPoints[0][0];$ry=$a.nativeGripPoints[1][1]-$a.nativeGripPoints[0][1];$len=[Math]::Sqrt($rx*$rx+$ry*$ry);$rx/=$len;$ry/=$len;$fx=-$ry;$fy=$rx
$script:PickupOffset=(' {0:F3} {1:F3}' -f ($fx*45+$rx*15),($fy*45+$ry*15))
if(!$SignsOnly){
 if(!$SkipSoft){
 $id=New-Bottle;$ground=(Target $id).tracking;Aim $ground .25 1200;Grab-Bottle $id
 $low=@($ground[0],($ground[1]+.07),$ground[2]);Move-Bottle $id $low 900
 Begin-Clip '01-soft-drop';Event 'soft release';$before=(Get-Addon).bottleBreaks;Send-XR @('grip r 0');Start-Sleep -Seconds 2
 Send-XR @('hand r to grip .35 -.15 -.2 5 55 5 800');Start-Sleep -Milliseconds 900
 Assert-Equal (Get-Addon).bottleBreaks $before 'Filmed gentle release leaves bottle intact';Save-Eyes 'bottle-soft-film';End-Clip
 }
 $id=New-Bottle;Grab-Bottle $id
 Send-XR @('head to 0 1.1 0 0 -65 0 1400','hand r to grip .18 .9 -.4 0 55 4 1400');Start-Sleep -Milliseconds 1600
 Begin-Clip '02-floor-smash';$before=(Get-Addon).bottleBreaks;Event 'throw down'
 Send-XR @('hand r to grip .1 .05 -.95 0 30 3 380','grip r squeeze 205');Start-Sleep -Milliseconds 900
 Send-XR @('hand r to grip .3 .55 -.2 5 55 4 1000');Start-Sleep -Milliseconds 1200
 Send-Game @('sc_glass_state')|Out-Null;$glass=@(Get-Content "$script:Session\glass-state.json" -Raw|ConvertFrom-Json)[0];Start-Sleep -Milliseconds 800
 Assert-Equal (Get-Addon).bottleBreaks ($before+1) 'Filmed thrown bottle shatters on ground';Save-Eyes 'bottle-floor-film';End-Clip
}
if($GroundOnly){return}
Park-Hands
$yaw=[Math]::Atan2($fx,$fy)*180/[Math]::PI
$sign=Get-Native ($fn+('"" 3 0 {0:F3} {1:F3} {2:F3}' -f $yaw,($fx*145),($fy*145)))
$signId=('{0:X8}' -f [int32]$sign);Start-Sleep -Milliseconds 1000
$t=Target $signId;$board=@($t.tracking[0],($t.tracking[1]+.60),$t.tracking[2]);Aim $board 1.1 1200;Save-Eyes 'bottle-sign-stage'
for($shot=1;$shot -le 3;$shot++){
 $id=New-Bottle;Grab-Bottle $id
 Send-XR @('hand r to grip .20 .65 -.28 0 55 4 1200');Start-Sleep -Milliseconds 1400
 Aim $board 1.1 700
 $start=(Target $id).tracking;$pose=(Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json).handR.pos
 $duration=.4;$u=.55;$s=3*$u*$u-2*$u*$u*$u;$derivative=6*$u*(1-$u)/$duration;$dz=-.9
 $flight=($board[2]-($start[2]+$s*$dz))/($derivative*$dz)
 if($flight -le 0 -or $flight -gt 1){throw "Invalid sign flight time $flight"}
 $dx=($board[0]-$start[0])/($s+$derivative*$flight)
 $dy=($board[1]-$start[1]+4.905*$flight*$flight)/($s+$derivative*$flight)
 $end=@(($pose[0]+$dx),($pose[1]+$dy),($pose[2]+$dz))
 @{sign=$signId;board=$board;start=$start;pose=$pose;end=$end;flight=$flight;signTarget=$t;forward=@($fx,$fy);yaw=$yaw}|ConvertTo-Json -Depth 5|Set-Content "$script:Film\throw-$shot-parameters.json"
 Begin-Clip ("0{0}-sign-smash" -f ($shot+2));$before=(Get-Addon).bottleBreaks;Event 'throw at sign'
 Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 0 40 4 400" -f $end[0],$end[1],$end[2]),'grip r squeeze 220');Start-Sleep -Milliseconds 1000
 Send-XR @('hand r to grip .3 .55 -.2 5 55 4 1100');Start-Sleep -Milliseconds 1300
 $a=Get-Addon;$a|ConvertTo-Json -Depth 10|Set-Content "$script:Film\sign-$shot-state.json"
 Assert-Equal $a.bottleBreaks ($before+1) "Sign throw $shot caused native hard-impact break"
 $impact=@(Get-Content "$script:GameLog"|Where-Object {$_ -like '*BOTTLE DECAL accepted=1*'}|Select-Object -Last 1)[0]
 if($impact -notmatch 'name=Stop '){throw "Throw did not hit the actual sign: $impact"}
 Save-Eyes "bottle-sign-$shot"
 if($shot -eq 3){Event 'watch native fade';Start-Sleep -Seconds 12;Save-Eyes 'bottle-sign-faded'}
 End-Clip
}
