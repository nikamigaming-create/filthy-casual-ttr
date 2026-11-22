$ErrorActionPreference='Stop'
. "$PSScriptRoot\runtime-helpers.ps1"
Start-Sleep -Seconds 12
& "$PSScriptRoot\dialogue-name-layout.ps1"
Send-XR @('input clear','head pose 0 1.1 0 0 -20 0')
Send-Game @('CloseAllMenus','load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-Game @('player.AddItem 00032C75 1','sc_aid')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 00032C75')|Out-Null;Start-Sleep -Milliseconds 1200
Send-XR @('hand r to grip .20 1.02 -.45 0 0 0 700','hand l to grip -.30 .9 -.40 0 0 0 700')
Start-Sleep -Milliseconds 900
Save-Eyes 'ttr115-whiskey-held'
$a=Get-Addon
Assert-Equal $a.nativeGrabHand 1 'Final candidate retains native right-hand bottle'
Assert-Equal $a.nativeGrabScale 1 'Final candidate preserves native item scale'
$before=Get-Native 'player.GetItemCount 00032C75'
for($i=0;$i -lt 5;$i++){
 $a=Get-Addon;if(!$a.heldFood){break}
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $p=@();for($j=0;$j -lt 3;$j++){$p+=($s.handR.pos[$j]+$a.mouthTracking[$j]-$a.heldTracking[$j])}
 Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 0 0 0 600" -f $p[0],$p[1],$p[2]));Start-Sleep -Milliseconds 1000
}
Assert-Equal (Get-Addon).heldFood $false 'Final candidate drinks once with native handoff'
Assert-Equal (Get-Native 'player.GetItemCount 00032C75') $before 'Held bottle consumes without a second inventory debit'
Send-XR @('hand r to grip .20 1.02 -.45 0 0 0 700');Start-Sleep -Milliseconds 900
$a=Get-Addon
Assert-Equal ($a.emptyBottleRef -ne 0 -and $a.nativeGrabRef -eq $a.emptyBottleRef -and $a.aidGripLatch) $true 'Empty bottle survives the near-wall staged handoff'
Assert-Equal $a.nativeGrabBase 0x5B6CE 'Correct whiskey empty is held after drinking'
Save-Eyes 'ttr115-empty-held'
Write-Output 'PASS final candidate dialogue head frame and native bottle handoff'
