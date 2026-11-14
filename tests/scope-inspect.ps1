. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 6
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 500}
Park-Hands
Send-Game @('player.AddItem 00004353 1','player.AddItem 0006B53C 40','sc_weapons') | Out-Null
Start-Sleep -Milliseconds 500
Send-Game @('sc_pick 00004353')|Out-Null
Start-Sleep -Seconds 1
Assert-Equal (Get-Addon).weaponHand 1 'Native sniper equipped'
Send-XR @('hand r grip pose 0.032 0.93 -0.40 0 55 0','hand l grip pose -0.06 0.89 -0.60 0 55 -20')
foreach($z in '-0.30','-0.16','-0.08','-0.02','0.04'){
 $lz=[double]$z-0.28
 Send-XR @("hand r to grip 0.032 0.93 $z 0 55 0 650","hand l to grip -0.06 0.89 $lz 0 55 -20 650")
 Start-Sleep -Seconds 1
 Save-Eyes ("scope-pullback-"+$z.Replace('.','p'))
}
Send-XR @('head to 0 1.1 0 10 0 0 500');Start-Sleep -Seconds 1
Save-Eyes 'scope-pullback-headturn'
Send-XR @('head to 0 1.1 0 0 0 0 400','hand r to grip 0.032 0.93 -0.18 0 55 0 400','video on')
$s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
$record=Start-Process -FilePath 'C:\Users\nbrys\AppData\Local\Programs\Python\Python311\python.exe' -ArgumentList @("`"$PSScriptRoot\record-eyes.py`"",'--pid',$s.pid,'--out',"`"$script:Repo\evidence\scope-twist-before.mp4`"",'--seconds',18,'--fps',30) -WindowStyle Hidden -PassThru
try {
 foreach($pose in @('0.032 0.93 -0.18 -25 55 -15','0.032 0.93 -0.10 25 55 15','0.032 0.93 -0.06 0 40 0','0.032 0.93 -0.18 0 70 0','0.032 0.93 -0.30 -25 55 15','0.032 0.93 -0.10 25 55 -15','0.032 0.93 -0.02 0 55 0')){
  Send-XR @("hand r to grip $pose 1800");Start-Sleep -Milliseconds 2100
 }
 if(!$record.WaitForExit(7000)){throw 'Scope recorder exceeded bounded duration'}
} finally {Send-XR @('video off');if(!$record.HasExited){Stop-Process -Id $record.Id -Force}}
