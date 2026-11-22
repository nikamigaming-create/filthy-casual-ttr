. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Send-Game @('player.AddItem 0001519B 3','player.damageav health 50') | Out-Null
function Pick-Food {
 Park-Hands
 Send-Game @('sc_aid') | Out-Null
 Start-Sleep -Milliseconds 400
 for($page=0;$page -lt 4;$page++){
  $a=Get-Addon
  $p=@($a.previews|Where-Object form -eq 86427)
  if($p.Count){Touch-Slot $p[0].slot 0 | Out-Null;Start-Sleep -Milliseconds 700;return}
  Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300
 }
 throw 'Food not in fixture'
}
$records=@()
foreach($nativeGrip in @($false,$true)){
 Pick-Food
 if($nativeGrip){Send-XR @('grip r 1');Start-Sleep -Milliseconds 300}
 $before=Get-Addon;$count=Get-Native 'player.GetItemCount 0001519B';$health=Get-Native 'player.GetAV Health'
 Send-XR @('btn a down');Start-Sleep -Milliseconds 2200
 Send-XR @('btn a up');Start-Sleep -Seconds 1
 $after=Get-Addon
 $records+=@{physicalGrip=$nativeGrip;beforeRef=$before.nativeGrabRef;afterRef=$after.nativeGrabRef;beforeCount=$count;afterCount=(Get-Native 'player.GetItemCount 0001519B');beforeHealth=$health;afterHealth=(Get-Native 'player.GetAV Health');manualBefore=$before.manualReload;manualAfter=$after.manualReload;menu=$after.menu}
 Assert-Equal $after.manualReload $before.manualReload 'Holding native A with food does not change reload mode'
 Send-XR @('grip r 0');Start-Sleep -Milliseconds 300
}
Send-XR @('btn y press 180');Start-Sleep -Milliseconds 500
Assert-Equal (Get-Native 'MenuMode') 1 'Y remains native Pause'
Send-Game @('CloseAllMenus') | Out-Null
$records|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\evidence\native-consume.json"
$records|ConvertTo-Json -Depth 6|Write-Output
