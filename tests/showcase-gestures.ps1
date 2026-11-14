. "$PSScriptRoot\casual-guide-helpers.ps1"
$script:Guide=Join-Path $script:Repo 'outputs\human-showcase'
Load-Guide
Send-Game @('player.SetWeaponOut 0')|Out-Null
Send-XR @('input clear','head to 0 1.1 0 0 -30 0 1200','hand l curve grip -0.22 0.94 -0.34 -10 55 -12 1800 -0.025 0.05 0','hand r curve grip 0.18 0.96 -0.35 40 55 10 1900 0.025 0.05 0')
Start-Sleep -Seconds 3
Start-Take '03-gestures' 65
try {
 Mark 'Hands alive';Start-Sleep -Seconds 3
 Mark 'Gentle finger curl';Send-XR @('thumbrest l on','thumbrest r on','touch l index on','touch r index on')
 foreach($g in .1,.25,.45,.65,.85,1,.85,.65,.45,.25,.1,0){Send-XR @("grip l $g","grip r $g");Start-Sleep -Milliseconds 160}
 Start-Sleep -Seconds 1
 Mark 'Point';Send-XR @('grip r 1','trigger r 0','touch r index off','thumbrest r on','hand r curve grip 0.15 0.98 -0.34 65 55 8 1800 0.02 0.04 0');Start-Sleep -Seconds 4
 $a=Get-Addon;Assert-Equal ($a.fingerGestures[1].index -lt .1) $true 'Point index is extended'
 Send-XR @('hand r curve grip 0.18 0.99 -0.36 45 65 12 2000 0.015 0.03 0');Start-Sleep -Seconds 3
 Mark 'Thumbs up';Send-XR @('touch r index on','thumbrest r off','touch r stick off','touch r primary off','touch r secondary off','hand r curve grip 0.19 0.99 -0.32 -4 55 -16 1800 0.015 0.04 0');Start-Sleep -Seconds 5
 Assert-Equal ((Get-Addon).fingerGestures[1].lift -gt .98) $true 'Thumb lift reached'
 Mark 'Relaxed wave';Send-XR @('grip r 0','touch r index off','hand r curve grip 0.25 1.00 -0.32 8 55 -50 1500 0.02 0.03 0');Start-Sleep -Milliseconds 1800
 Send-XR @('hand r curve grip 0.17 1.00 -0.33 -8 55 -35 1500 -0.02 0.02 0');Start-Sleep -Milliseconds 1800
 Send-XR @('hand l curve grip -0.23 0.78 -0.20 -8 55 -8 1600 -0.02 0.04 0','hand r curve grip 0.22 0.80 -0.23 8 55 8 1800 0.02 0.04 0');Start-Sleep -Seconds 4
 Finish-Take
}finally{Stop-Take}
