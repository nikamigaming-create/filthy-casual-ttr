. "$PSScriptRoot\runtime-helpers.ps1"
# Acceptance: existing name above speaker, choices beside the silhouette, both
# eyes, close/normal/far distances, both sides, head motion and conversation exit.
# No activation of a topic: A selects barter in this fixture, not a greeting skip.
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 6
for($notice=0;$notice -lt 3 -and (Get-Addon).nativeTopMenu -eq 1001;$notice++){
 Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600
}
Park-Hands
$results=@()
foreach($distance in @(60,180)){
 Send-Game @('CloseAllMenus',"player.MoveTo 00104C79 $distance 0 0",'player.SetAngle z 270')|Out-Null
 Send-XR @('head pose 0 1.1 0 0 0 0')
 Start-Sleep -Milliseconds 600
 Send-Game @('expr Call ({ref actor} => actor.Activate player) (StringToRef "00104C79")')|Out-Null
 Start-Sleep -Seconds 2
 $a=Get-Addon
 $faceYaw=[Math]::Atan2(-($a.speakerTracking[0]-$a.nativeHeadTracking[0]),-($a.speakerTracking[2]-$a.nativeHeadTracking[2]))*180/[Math]::PI
 foreach($side in @(-15,15)){
  Send-Game @('CloseAllMenus')|Out-Null
  Start-Sleep -Milliseconds 350
  $yaw=$faceYaw+$side
  Send-XR @("head pose 0 1.1 0 $yaw -10 0")
  Send-Game @('expr Call ({ref actor} => actor.Activate player) (StringToRef "00104C79")')|Out-Null
  Start-Sleep -Seconds 2
  $a=Get-Addon
  Assert-Equal $a.nativeTopMenu 1009 'Actual dialogue remains open'
  Assert-Equal $a.speakerNameActive $true 'Speaker bone supplies live name position'
  Assert-Equal $a.speakerNameCrop.ready $true 'Existing name isolated from choices'
  $tag="dialogue-clearance-$distance-$side"
  Save-Eyes $tag
  $j=Get-Content "$script:Repo\evidence\$tag.json" -Raw|ConvertFrom-Json
  $name=@($j.layers|Where-Object { $_.type -eq 'quad' -and $_.sizeM[1] -le .06 })
  Assert-Equal $name.Count 1 'Exactly one separate native name layer'
  for($axis=0;$axis -lt 3;$axis++){
   Assert-Equal ([Math]::Abs($name[0].pose[$axis]-$a.speakerNameTracking[$axis]) -lt .06) $true 'Name layer follows speaker head anchor'
  }
  $before=$a.dialoguePlacements
  Send-XR @("head to .05 1.1 .03 $($yaw+8) -10 0 800")
  Start-Sleep -Milliseconds 1000
  $a=Get-Addon
  Assert-Equal $a.dialoguePlacements $before 'Choices do not follow head movement'
  Save-Eyes "$tag-moved"
  $results+=@{distance=$distance;side=$side;state=$a;capture=$tag}
 }
}
Send-Game @('CloseAllMenus')|Out-Null
Start-Sleep -Milliseconds 500
Assert-Equal (Get-Addon).speakerNameActive $false 'Name removed when conversation ends'
Save-Eyes 'dialogue-clearance-closed'
$results|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\dialogue-clearance-matrix.json"
