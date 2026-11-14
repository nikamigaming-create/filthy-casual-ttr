. "$PSScriptRoot\runtime-helpers.ps1"
. "$PSScriptRoot\casual-guide-helpers.ps1"
$script:CaptureAddonPath='D:\TTWVR\MO2\mods\TTWVR - Vertibird Development\NVSE\Plugins\SeatedControls.dll'
function Frame {
 $a=Get-Addon;$l=$a.nativeGripPoints[0];$r=$a.nativeGripPoints[1];$tl=$a.nativeGripTracking[0];$tr=$a.nativeGripTracking[1]
 $dx=$r[0]-$l[0];$dy=$r[1]-$l[1];$len=[Math]::Sqrt($dx*$dx+$dy*$dy);if($len -lt 20){throw 'No valid walking basis'}
 $rx=$dx/$len;$ry=$dy/$len;$fx=-$ry;$fy=$rx;$u=$len/($tr[0]-$tl[0]);$h=$a.nativeHeadTracking
 return @{rx=$rx;ry=$ry;fx=$fx;fy=$fy;u=$u;x=($l[0]+($h[0]-$tl[0])*$u*$rx-($h[2]-$tl[2])*$u*$fx);y=($l[1]+($h[0]-$tl[0])*$u*$ry-($h[2]-$tl[2])*$u*$fy);z=($l[2]+($h[1]-$tl[1])*$u)}
}
function Face([double]$X,[double]$Y,[double]$Z,[int]$Ms=1400){
 $f=Frame;$dx=$X-$f.x;$dy=$Y-$f.y;$right=$dx*$f.rx+$dy*$f.ry;$fore=$dx*$f.fx+$dy*$f.fy
 $yaw=[Math]::Atan2(-$right,$fore)*180/[Math]::PI;$pitch=[Math]::Atan2($Z-$f.z,[Math]::Sqrt($dx*$dx+$dy*$dy))*180/[Math]::PI
 $pitch=[Math]::Clamp($pitch,-35,25)
 Send-XR @(("head to 0 1.1 0 {0:F4} {1:F4} 0 {2}" -f $yaw,$pitch,$Ms));return $yaw
}
function Walk([double]$X,[double]$Y,[double]$LookX,[double]$LookY,[double]$LookZ,[int]$Limit=15){
 $clock=[Diagnostics.Stopwatch]::StartNew();$last=1e10;$stuck=0;$detours=0;$lastPos=$null
 try {
  while($clock.Elapsed.TotalSeconds -lt $Limit){
   $f=Frame;if($lastPos -and ([Math]::Abs($f.x-$lastPos.x)+[Math]::Abs($f.y-$lastPos.y) -gt 1000)){throw 'Unexpected player relocation during walk'};$lastPos=$f;$dx=$X-$f.x;$dy=$Y-$f.y;$dist=[Math]::Sqrt($dx*$dx+$dy*$dy)
   if($dist -lt 45){return}
   $yaw=Face $LookX $LookY $LookZ 650;$a=$yaw*[Math]::PI/180
   $rx=$f.rx*[Math]::Cos($a)+$f.fx*[Math]::Sin($a);$ry=$f.ry*[Math]::Cos($a)+$f.fy*[Math]::Sin($a)
   $fx=$f.fx*[Math]::Cos($a)-$f.rx*[Math]::Sin($a);$fy=$f.fy*[Math]::Cos($a)-$f.ry*[Math]::Sin($a)
   $speed=[Math]::Min(.42,[Math]::Max(.2,$dist/500));$sx=($dx*$rx+$dy*$ry)/$dist*$speed;$sy=($dx*$fx+$dy*$fy)/$dist*$speed
   Send-XR @(("stick l {0:F4} {1:F4}" -f $sx,$sy));Start-Sleep -Milliseconds 450
   $stuck=if($dist -gt $last-8){$stuck+1}else{0};$last=$dist
   if($stuck -ge 5){
    if($detours -ge 2){throw "Walking blocked near $($f.x),$($f.y); target $X,$Y"}
    Write-Output 'Native obstacle detour';$side=if($detours -eq 0){.45}else{-.45};Send-XR @("stick l $side 0");Start-Sleep -Milliseconds 1200;Send-XR @('stick l 0 0');$detours++;$stuck=0;$last=1e10
   }
  }
  throw "Walking exceeded its bounded time to $X,$Y"
 } finally {Send-XR @('stick l 0 0')}
}
function New-Take([string]$Name){$script:events=[Collections.Generic.List[object]]::new();Start-Take $Name 180}
function Bird {return @((Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 4'),(Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 5'),(Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 6'))}
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 0 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 7
for($i=0;$i -lt 4 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands;Send-Game @('tdetect','set GameHour to 12','player.SetWeaponOut 0','PVHGutsyREF.MoveTo PVHGutsyPost','PVHGutsyREF.Enable','set PVHGeneralQuest.CanFly to 1','StopQuest PVHQuest1')|Out-Null
Start-Sleep -Seconds 1
Assert-Equal (Get-Native 'PVHGutsyREF.GetInCell PVHVertibirdInteriorCell') 1 'Native acquisition boarding gate is ready'
# Stage at the owned Ivanpah Dry Lake marker before querying loaded terrain.
Send-Game @('player.MoveTo 00145D50')|Out-Null;Start-Sleep -Seconds 4
for($i=0;$i -lt 6 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 700}
Assert-Equal (Get-Addon).menu 0 'Arrival notification dismissed before walking'
# Select a reasonably level terrain patch before any recording or route claim.
$best=$null
foreach($cx in -58000,-56000,-54000){foreach($cy in -77000,-75000){
 $heights=@();foreach($dx in -1100,0,1100){foreach($dy in -1100,0,1100){$heights+=Get-Native ("GetTerrainHeight {0} {1}" -f ($cx+$dx),($cy+$dy))}}
 $range=($heights|Measure-Object -Maximum).Maximum-($heights|Measure-Object -Minimum).Minimum
 if(!$best -or $range -lt $best.range){$best=@{x=$cx;y=$cy;range=$range}}
}}
$startY=$best.y-1600;$ground=Get-Native ("GetTerrainHeight {0} {1}" -f $best.x,$startY)
Write-Output ("FIELD {0} {1}, relief {2}" -f $best.x,$best.y,$best.range)
Send-Game @("player.SetPos X $($best.x)","player.SetPos Y $startY","player.SetPos Z $($ground+10)")|Out-Null;Start-Sleep -Seconds 3
Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 2'|Out-Null;Start-Sleep -Seconds 2
$ground=Get-Native ("GetTerrainHeight {0} {1}" -f $best.x,$best.y)
Get-Native ('Call (CompileScript "FCVertibirdGround.txt") '+($ground-40))|Write-Output
Start-Sleep -Seconds 1
Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 8'|Out-Null
$landingDeadline=[DateTime]::UtcNow.AddSeconds(45)
do{Start-Sleep -Seconds 1;$parked=Get-Native 'PVHVertibirdREF.GetVariable "State"'}while($parked -ne 1 -and [DateTime]::UtcNow -lt $landingDeadline)
Assert-Equal $parked 1 'Original arrival animation finishes before walkaround'
for($i=0;$i -lt 4 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 700}
Assert-Equal (Get-Addon).menu 0 'No modal panel blocks the tour'
$b=Bird;$bx=$b[0];$by=$b[1];$bz=$b[2];Face $bx $by ($bz+230) 2000|Out-Null;Start-Sleep -Seconds 3
Save-Eyes 'vertibird-tour-wide'
Get-Native 'Call (CompileScript "FCVertibirdTour.txt") 3'|Out-Null;Start-Sleep -Seconds 2;Send-XR @('btn a press 180');Start-Sleep -Seconds 6
Assert-Equal (Get-Native 'player.GetInCell PVHVertibirdInteriorCell') 1 'Resume fixture boards original aircraft'
Get-Native 'Call (CompileScript "FCVertibirdTest.txt") 0'|Out-Null;Start-Sleep -Seconds 2
Get-Native 'Call (CompileScript "FCVertibirdTest.txt") 1'|Out-Null;Send-XR @('head to 0 1.1 0 0 -5 0 1600');Start-Sleep -Seconds 2;Send-XR @('btn a press 180');Start-Sleep -Seconds 12
Assert-Equal (Get-Native 'GetVariable "Piloting" (GetFormFromMod "Player Vertibird.esm" "000AE7")') 1 'Resume fixture starts native flight'
Send-XR @('btn y press 180','hand l to grip -0.25 .67 -.22 0 50 0 1800','hand r to grip .22 .67 -.22 0 50 0 1800','thumbrest l on','thumbrest r on','touch l index on','touch r index on');Start-Sleep -Seconds 3
New-Take 'vertibird-fallout-flight-tour'
try {
 Mark 'Fallout cockpit forward view';Save-Eyes 'vertibird-fallout-pilot'
 foreach($yaw in -45,-90,-125,-60,0,45,90,125,60,0){
  Mark "Cockpit look $yaw";Send-XR @("head to 0 1.1 0 $yaw -8 0 3200");Start-Sleep -Milliseconds 4700
 }
 Mark 'Overhead controls';Send-XR @('head to 0 1.1 0 25 55 0 2600');Start-Sleep -Milliseconds 4200
 Mark 'Seated pilot';Send-XR @('head to 0 1.1 0 0 -60 0 3800');Start-Sleep -Milliseconds 5200
 Send-XR @('head to 0 1.1 0 0 -5 0 2400');Start-Sleep -Milliseconds 3200
 Mark 'Climb';Send-XR @('btn a down');Start-Sleep -Seconds 3;Send-XR @('btn a up')
 Mark 'Forward flight';Send-XR @('stick l 0 .4');Start-Sleep -Seconds 4;Send-XR @('stick l 0 0');Start-Sleep -Seconds 2
 Save-Eyes 'vertibird-fallout-flight'
 Mark 'Land';Send-XR @('btn b press 350');$end=[DateTime]::UtcNow.AddSeconds(25)
 do{Start-Sleep -Seconds 1;$f=Get-Native 'GetVariable "Piloting" (GetFormFromMod "Player Vertibird.esm" "000AE7")'}while($f -eq 1 -and [DateTime]::UtcNow -lt $end)
 Assert-Equal $f 0 'Native landing completes';Assert-Equal (Get-Native 'player.GetDead') 0 'Pilot survives';Finish-Take
}finally{Stop-Take}
New-Take 'vertibird-exit-walkaround-after'
try {
 Mark 'Leave the landed aircraft'
 # Native flight landing places the player outside at PlayerExit.
 Assert-Equal (Get-Native 'player.GetInCell PVHVertibirdInteriorCell') 0 'Native landing exits beside the aircraft'
 $b=Bird;$bx=$b[0];$by=$b[1];$bz=$b[2]
 Walk ($bx-1100) ($by+150) $bx $by ($bz+230)
 for($i=1;$i -le 8;$i++){
  $theta=(-90+$i*45)*[Math]::PI/180;Mark "Landed walkaround angle $($i*45)"
  Walk ($bx+1100*[Math]::Sin($theta)) ($by+1100*[Math]::Cos($theta)) $bx $by ($bz+230) 15
  Start-Sleep -Seconds 2
  Mark "Completed viewpoint $($i*45)"
 }
 Save-Eyes 'vertibird-tour-landed';Finish-Take
}finally{Stop-Take}
