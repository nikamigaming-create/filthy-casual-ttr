. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -15 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('player.SetWeaponOut 0','set GameHour to 12','player.SetPos X -72500','player.SetPos Y 2000')|Out-Null
Start-Sleep -Seconds 2
foreach($drink in '00032C75','000151A1','0001519E','00103B1E'){
 Send-Game @("player.AddItem $drink 1",'sc_aid')|Out-Null;Start-Sleep -Milliseconds 350
 Send-Game @("sc_pick $drink")|Out-Null;Start-Sleep -Milliseconds 1200
 $a=Get-Addon
 Assert-Equal $a.nativeGrabHand 1 'Drink uses the normal right hand'
 Assert-Equal $a.heldFood $true 'Bottled drink supports mouth consumption'
 Send-XR @('hand r to grip .45 .65 -.65 0 0 0 700');Start-Sleep -Milliseconds 950
 $full=$a.nativeGrabRef;$before=Get-Native "player.GetItemCount $drink"
 for($i=0;$i -lt 5;$i++){
  $a=Get-Addon;if($a.heldFood -eq $false){break}
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $p=@();for($j=0;$j -lt 3;$j++){$p+=($s.handR.pos[$j]+$a.mouthTracking[$j]-$a.heldTracking[$j])}
  Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 0 0 0 600" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 850
 }
 Start-Sleep -Milliseconds 1000
 $a=Get-Addon;$a|ConvertTo-Json -Depth 15|Set-Content "$script:Repo\work\vehicle-controls\empty-$drink.json"
 Assert-Equal (Get-Native "player.GetItemCount $drink") $before 'Exactly the held drink consumed; bag unchanged'
 Assert-Equal ($a.emptyBottleRef -ne 0 -and $a.nativeGrabRef -eq $a.emptyBottleRef -and $a.aidGripLatch) $true 'Real empty bottle replaces consumed drink in the hand'
 $id=('{0:X8}' -f [uint32]$a.emptyBottleRef)
 $traceStart=@(Get-Content $script:GameLog).Count
 Send-Game @("sc_track_bottle $id")|Out-Null
 $expected=@{'00032C75'=0x5B6CE;'000151A1'=0x5B6CD;'0001519E'=0x22103;'00103B1E'=0x103B1B}[$drink]
 Assert-Equal $a.nativeGrabBase $expected 'Held reference has the correct empty-bottle base'
 Write-Output "Replacement pose: position=$($a.emptyPoseError) rotation=$($a.emptyRotationError)"
 Assert-Equal ($a.emptyPoseError -lt .75 -and $a.emptyRotationError -lt .06) $true 'Replacement preserves the original hand-relative bottle pose'
 Send-XR @('hand r to grip .2 .85 -.4 0 55 0 900');Start-Sleep -Milliseconds 1100
 Save-Eyes "empty-bottle-$drink"
 for($i=0;$i -lt 4 -and (Get-Native 'MenuMode');$i++){Send-XR @('btn a press 150');Start-Sleep -Milliseconds 450}
 Send-XR @('grip r 1');Start-Sleep -Milliseconds 250
 Assert-Equal (Get-Addon).aidGripLatch $false 'Squeeze returns the empty to normal native grip'
 Send-XR @('hand r to grip .6 1 -.8 50 30 0 500','grip r squeeze 250');Start-Sleep -Milliseconds 950
 Assert-Equal (Get-Addon).nativeGrabRef 0 'Native hand ownership released'
 Start-Sleep -Milliseconds 1700
 $samples=@(Get-Content $script:GameLog | Select-Object -Skip $traceStart | Select-String -Pattern ('BOTTLE POSE ref='+$id+' held=0 root=([^ ]+) ([^ ]+) ([^ ]+)') | ForEach-Object { $_.Matches[0].Groups[1..3].Value -join ',' })
 Assert-Equal (@($samples|Select-Object -Unique).Count -gt 8) $true 'Visible released bottle follows native physics continuously'
 Save-Eyes "empty-bottle-released-$drink"
 Park-Hands
}

& python "$PSScriptRoot\check-bottle-release-trace.py" $script:GameLog --families 4
if($LASTEXITCODE -ne 0){throw 'Rendered bottle trajectory diverged from native physics'}
