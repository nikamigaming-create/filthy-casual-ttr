. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('head to 0 1.1 0 0 -20 0 700','hand l grip pose -0.3 0.95 -0.4 0 0 0')
$script:cuffAngles=''
foreach($angles in @('0 0 180','0 55 180','0 0 90','0 0 -90','90 0 0','-90 0 0')){
 Send-XR @("hand r to grip 0.1 0.95 -0.35 $angles 600")
 Start-Sleep -Milliseconds 800
 $a=Get-Addon
 Write-Output "Cuff pose $angles : attached=$($a.wrist.attached) facing=$($a.wrist.facing)"
 if($a.wrist.attached -and $a.wrist.facing){$script:cuffAngles=$angles;break}
}
Assert-Equal ($script:cuffAngles.Length -gt 0) $true 'Dorsal cuff faces the seated viewer'
Assert-Equal (@($a.wristPlungers|Where-Object found).Count) 5 'All five native plunger branches loaded'
Save-Eyes 'cuff-native-fit'
$before=Get-Addon
Send-XR @('hand l grip pos -0.26 0.95 -0.4')
Start-Sleep -Milliseconds 250
$after=Get-Addon
$dx=$after.wrist.tip[0]-$before.wrist.tip[0];$dy=$after.wrist.tip[1]-$before.wrist.tip[1]
$length=[Math]::Sqrt($dx*$dx+$dy*$dy)
Assert-Equal ($length -gt 1) $true 'Measured controller-to-native horizontal basis'
$script:cuffRx=$dx/$length;$script:cuffRy=$dy/$length
function Move-CuffTip([double[]]$Target,[int]$Duration=350){
 for($attempt=0;$attempt -lt 3;$attempt++){
  $a=Get-Addon;$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $ex=$Target[0]-$a.wrist.tip[0];$ey=$Target[1]-$a.wrist.tip[1];$ez=$Target[2]-$a.wrist.tip[2]
  if([Math]::Sqrt($ex*$ex+$ey*$ey+$ez*$ez) -lt .018){break}
  $px=$s.handL.pos[0]+($script:cuffRx*$ex+$script:cuffRy*$ey)/69.99125
  $py=$s.handL.pos[1]+$ez/69.99125
  $pz=$s.handL.pos[2]-(-$script:cuffRy*$ex+$script:cuffRx*$ey)/69.99125
  Send-XR @(("hand l to grip {0:F6} {1:F6} {2:F6} 0 0 0 {3}" -f $px,$py,$pz,$Duration))
  Start-Sleep -Milliseconds ($Duration+100)
 }
}
function Press-Cuff([int]$Index){
 $a=Get-Addon;$target=$a.wrist.targets[$Index];$normal=$a.wrist.normal
 $above=0..2|ForEach-Object {$target[$_]+$normal[$_]*1.4}
 Move-CuffTip $above 500
 Start-Sleep -Milliseconds 450
 $a=Get-Addon
 Assert-Equal $a.openHand -1 'Hover above cuff does not open a wheel'
 $pressed=0..2|ForEach-Object {$a.wrist.targets[$Index][$_]-$a.wrist.normal[$_]*.105}
 Move-CuffTip $pressed 350
 Start-Sleep -Milliseconds 150
 return Get-Addon
}
function Withdraw-Cuff {
 $a=Get-Addon;$normal=$a.wrist.normal
 $away=0..2|ForEach-Object {$a.wrist.tip[$_]+$normal[$_]*4}
 Move-CuffTip $away 450
 Start-Sleep -Milliseconds 450
}
$records=@()
$a=Press-Cuff 3;$records+=@{action='settings';state=$a}
Assert-Equal $a.heightMenu $true 'Physical gear press opens height settings'
Assert-Equal ($a.wristPlungers[4].depression -gt .5) $true 'Native gear plunger physically depresses'
Save-Eyes 'cuff-native-pressed'
Withdraw-Cuff
Send-XR @('btn b press 180');Start-Sleep -Milliseconds 400
Assert-Equal (Get-Addon).heightMenu $false 'B closes height settings'
$original=(Get-Addon).manualReload
$a=Press-Cuff 2;$records+=@{action='reload';state=$a}
Assert-Equal $a.manualReload (!$original) 'Physical reload press switches actual native mode'
Save-Eyes 'cuff-native-mode'
Withdraw-Cuff
$a=Press-Cuff 2
Assert-Equal $a.manualReload $original 'Second deliberate press restores original reload preference'
Withdraw-Cuff
$a=Press-Cuff 0;$records+=@{action='weapons';state=$a}
Assert-Equal $a.openHand 1 'Physical weapon button opens weapon wheel'
Withdraw-Cuff
Save-Eyes 'cuff-native-weapons'
Touch-Slot 1 0|Out-Null
Start-Sleep -Milliseconds 800
Assert-Equal (Get-Addon).weaponHand 1 'Weapon selection hands native item to right hand'
Send-XR @('grip r 1');Start-Sleep -Milliseconds 250
Assert-Equal (Get-Addon).weaponHand -1 'Deliberate squeeze returns weapon handling to native mode'
Send-XR @('grip r 0');Start-Sleep -Milliseconds 300
Send-XR @("hand r to grip 0.1 0.95 -0.35 $script:cuffAngles 500")
Start-Sleep -Milliseconds 650
Withdraw-Cuff
$a=Press-Cuff 1;$records+=@{action='aid';state=$a}
Assert-Equal $a.openHand 0 'Physical supplies button opens consumable wheel'
Withdraw-Cuff
Save-Eyes 'cuff-native-aid'
Send-XR @('btn b press 180');Start-Sleep -Milliseconds 300
Assert-Equal (Get-Addon).openHand -1 'B closes consumable wheel'
Send-XR @('btn y press 180');Start-Sleep -Milliseconds 300
Assert-Equal (Get-Native 'MenuMode') 1 'Native Y still opens Pause outside addon panels'
Send-Game @('CloseAllMenus')|Out-Null
$records|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\wrist-cuff.json"
