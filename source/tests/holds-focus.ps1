. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600}
Park-Hands
Send-Game @('player.SetWeaponOut 0') | Out-Null
Send-XR @('input clear','head to 0 1.1 0 0 -25 0 500','hand l to grip -0.20 0.95 -0.40 0 55 0 500','hand r to grip 0.20 0.95 -0.40 0 55 0 500')
Start-Sleep -Milliseconds 900
function Pair([string]$Left,[string]$Right,[int]$Hold=750){
 Send-XR @("btn $Left down","btn $Right down")
 Start-Sleep -Milliseconds $Hold
 Send-XR @("btn $Left up","btn $Right up")
 Start-Sleep -Milliseconds 250
}
function Back {Send-XR @('btn b press 100');Start-Sleep -Milliseconds 600}
Send-XR @('touch r stick on','thumbrest l on');Start-Sleep -Milliseconds 600
Send-XR @('state visible');Start-Sleep -Milliseconds 500
Send-XR @('state focused');Start-Sleep -Milliseconds 1200
Assert-Equal (Get-Addon).openHand -1 'Interrupted thumbrest hold does not resume after focus loss'
Send-XR @('thumbrest l off','btn y press 100');Start-Sleep -Milliseconds 500
Assert-Equal ((Get-Addon).menu -ne 0) $true 'Y still opens native Pause without combo delay'
Send-XR @('btn b press 100');Start-Sleep -Milliseconds 500
Assert-Equal (Get-Addon).menu 0 'B still closes native Pause'
Write-Output 'PASS: focus cancellation and native Pause/Back'
