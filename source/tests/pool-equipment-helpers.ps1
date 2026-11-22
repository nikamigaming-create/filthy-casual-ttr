. "$PSScriptRoot\runtime-helpers.ps1"
function Get-PoolEquipment {
 for($read=0;$read -lt 12;$read++){
  try{$e=Get-Content "$script:Session\pool-equipment.json" -Raw|ConvertFrom-Json;if($e.available -and $e.items.Count -eq 24){return $e}}catch{}
  Start-Sleep -Milliseconds 40
 }
 throw 'No complete current equipment snapshot'
}
function Pool-Palm([int]$Hand,[double[]]$Target,[int]$Duration=700,[double[]]$Angles=@(0,40,0)){
 $h=if($Hand){'r'}else{'l'}
 for($try=0;$try -lt 4;$try++){
  $e=Get-PoolEquipment;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $state=if($Hand){$s.handR}else{$s.handL}
  $p=@(0.,0.,0.);$error=0.
  for($j=0;$j -lt 3;$j++){$d=$Target[$j]-$e.palms[$Hand][$j];$p[$j]=$state.pos[$j]+$d;$error+=$d*$d}
  if($error -lt .0001){return}
  Send-XR @(("hand {0} curve grip {1:F5} {2:F5} {3:F5} {4:F3} {5:F3} {6:F3} {7} 0 .025 0" -f $h,$p[0],$p[1],$p[2],$Angles[0],$Angles[1],$Angles[2],$Duration))
  Start-Sleep -Milliseconds ($Duration+160)
 }
 $e=Get-PoolEquipment;$error=0.
 for($j=0;$j -lt 3;$j++){$error+=[Math]::Pow($Target[$j]-$e.palms[$Hand][$j],2)}
 Write-Output ("Palm residual {0:F3}m" -f [Math]::Sqrt($error))
}
function Pool-Look([double[]]$Target,[int]$Duration=1000){
 $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $dx=$Target[0]-$a.nativeHeadTracking[0];$dy=$Target[1]-$a.nativeHeadTracking[1];$dz=$Target[2]-$a.nativeHeadTracking[2]
 $yaw=[Math]::Atan2(-$dx,-$dz)*180/[Math]::PI
 $pitch=[Math]::Atan2($dy,[Math]::Sqrt($dx*$dx+$dz*$dz))*180/[Math]::PI
 Send-XR @(("head to {0:F3} {1:F3} {2:F3} {3:F3} {4:F3} 0 {5}" -f $s.head.pos[0],$s.head.pos[1],$s.head.pos[2],$yaw,$pitch,$Duration))
 Start-Sleep -Milliseconds ($Duration+100)
}
function Load-PoolEquipment {
 Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -15 0')
 Start-Sleep -Seconds 12
 Send-Game @('load SC_QA_fixture')|Out-Null
 Start-Sleep -Seconds 5
 for($i=0;$i -lt 3 -and (Get-Addon).nativeTopMenu -eq 1001;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600}
 Park-Hands
 Send-Game @('coc GSProspectorSaloonInterior')|Out-Null
 Start-Sleep -Seconds 4
 Send-Game @('CloseAllMenus','player.SetWeaponOut 0')|Out-Null
 Start-Sleep -Milliseconds 800
}
function Ensure-PoolGameplay {
 for($i=0;$i -lt 6;$i++){
  if((Get-Native 'MenuMode') -eq 0){return}
  Send-XR @('btn b press 180')
  Start-Sleep -Milliseconds 900
 }
 throw 'Native dialogue has not finished'
}
function Load-ReadyPool {
 Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -15 0')
 Start-Sleep -Seconds 12
 Send-Game @('load FC_QA_PoolGear_v4')|Out-Null
 Start-Sleep -Seconds 5
 for($i=0;$i -lt 3 -and (Get-Addon).nativeTopMenu -eq 1001;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600}
 Park-Hands
 Assert-Equal (Get-Native 'MenuMode') 0 'Pool test save loads directly into gameplay'
}

function Face-PoolItem([int]$Item){
 for($turn=0;$turn -lt 10;$turn++){
  $a=Get-Addon;$e=Get-PoolEquipment;$target=$e.items[$Item].tracking
  $bearing=[Math]::Atan2(-($target[0]-$a.nativeHeadTracking[0]),-($target[2]-$a.nativeHeadTracking[2]))*180/[Math]::PI
  if([Math]::Abs($bearing) -lt 25){return}
  $direction=-[Math]::Sign($bearing)
  Send-XR @("stick r $direction 0");Start-Sleep -Milliseconds 180
  Send-XR @('stick r center');Start-Sleep -Milliseconds 500
 }
 throw "Native body could not turn toward equipment $Item"
}

function Read-PoolPose {Get-Content "$script:Session\pool-weapon.json" -Raw | ConvertFrom-Json}
function Count-Shots {(Get-Content "$script:Session\pool-match-state.json" -Raw | ConvertFrom-Json).shots}
function Move-Bridge([double]$FromTip=.86) {
    $w=Read-PoolPose
    if(!$w.pairedSequence){throw "Missing equipped cue pose ($($w.poseState))"}
    $s=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
    $along=$FromTip
    $angles=$s.handL.ypr
    $target=@(0.,0.,0.);$distance=0.
    for($j=0;$j -lt 3;$j++){
        $target[$j]=$s.handL.pos[$j]+$w.tip[$j]-$along*$w.axis[$j]-$w.bridgeSocket[$j]
        $distance += [Math]::Pow($target[$j]-$s.head.pos[$j],2)
    }
    if($distance -gt .85*.85){throw "Approach exceeds arm reach ($distance squared metres)"}
    Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 550" -f $target[0],$target[1],$target[2],$angles[0],$angles[1],$angles[2]))
    Start-Sleep -Milliseconds 800
}
