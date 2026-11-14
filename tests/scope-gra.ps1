. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null
Start-Sleep -Seconds 6
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 500}
$equipped=Get-Native 'player.GetEquippedObject 5'
Write-Output ('Outdoor save equipped form: {0:X8}' -f [int]$equipped)
$weaponIds=@('0500084F')
# User clarified the fault was on the GRA rifle outdoors, not necessarily the
# weapon equipped when this autosave was written.
foreach($weapon in $weaponIds){
 Park-Hands
 Send-Game @("player.AddItem $weapon 1",'sc_weapons')|Out-Null
 Start-Sleep -Milliseconds 400
 Send-Game @("sc_pick $weapon")|Out-Null
 Start-Sleep -Seconds 1
 Assert-Equal (Get-Addon).weaponHand 1 "GRA $weapon equipped"
 Send-XR @('hand r grip pose 0.08 0.93 -0.65 0 55 0','hand l grip pose -0.10 0.88 -0.65 0 55 -20','video on')
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
 $record=Start-Process -FilePath 'C:\Users\nbrys\AppData\Local\Programs\Python\Python311\python.exe' -ArgumentList @("`"$PSScriptRoot\record-eyes.py`"",'--pid',$s.pid,'--out',"`"$script:Repo\evidence\scope-outdoor-$weapon-before.mp4`"",'--seconds',16,'--fps',30) -WindowStyle Hidden -PassThru
 try {
  foreach($pose in @('0.08 0.93 -0.65 -25 55 -20','0.08 0.93 -0.65 25 55 20','0.032 0.93 -0.65 0 40 0','0.032 0.93 -0.65 0 70 0','0.032 0.93 -0.40 -15 55 15','0.032 0.93 -0.18 15 55 -15','0.032 0.93 -0.08 0 55 0')){
   Send-XR @("hand r to grip $pose 1500");Start-Sleep -Milliseconds 1850
  }
  if(!$record.WaitForExit(7000)){throw 'GRA recorder timed out'}
 } finally {Send-XR @('video off');if(!$record.HasExited){Stop-Process -Id $record.Id -Force}}
 Save-Eyes "scope-gra-$weapon"
}
