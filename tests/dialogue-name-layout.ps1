. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 6
for($notice=0;$notice -lt 3 -and (Get-Addon).nativeTopMenu -eq 1001;$notice++){
 Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600
}
Park-Hands
Send-Game @('player.MoveTo 00104C79 110 0 0','player.SetAngle z 270')|Out-Null
Start-Sleep -Seconds 2
function Open-Conversation {
 Send-Game @('expr Call ({ref actor} => actor.Activate player) (StringToRef "00104C79")')|Out-Null
 Start-Sleep -Seconds 3
}
Open-Conversation
$a=Get-Addon
$dx=$a.speakerTracking[0]-$a.nativeHeadTracking[0]
$dz=$a.speakerTracking[2]-$a.nativeHeadTracking[2]
$faceYaw=[Math]::Atan2(-$dx,-$dz)*180/[Math]::PI
$angles=@()
foreach($side in @(-20,20)){
 Send-Game @('CloseAllMenus')|Out-Null
 Start-Sleep -Milliseconds 500
 $viewYaw=$faceYaw+$side
 Send-XR @("head pose 0 1.1 0 $viewYaw 0 0")
 Start-Sleep -Milliseconds 300
 Open-Conversation
 $a=Get-Addon
 Assert-Equal $a.speakerNameActive $true 'Native name anchored above actor'
 Assert-Equal $a.speakerNameCrop.ready $true 'Native name separated without duplicate'
 $angles+= $a.dialogueSideDegrees
 $a|Select-Object dialogueSideDegrees,speakerNameTracking,speakerNameCrop|ConvertTo-Json -Depth 5 -Compress
 Save-Eyes ('dialogue-layout-'+$side)
 # Greeting has reached the topic list; A would open barter here.
 Assert-Equal (Get-Addon).nativeTopMenu 1009 'Still testing dialogue, not barter'
 $before=(Get-Addon).dialoguePlacements
 $lookYaw=$viewYaw+5
 Send-XR @("head to 0.03 1.1 0 $lookYaw 0 0 700")
 Start-Sleep -Seconds 1
 Assert-Equal (Get-Addon).dialoguePlacements $before 'Conversation panel stays anchored during head turn'
}
Assert-Equal ([Math]::Sign($angles[0]) -ne [Math]::Sign($angles[1])) $true 'Dialogue chooses opposite sides when viewing room changes'
