$ErrorActionPreference='Stop'
. "$PSScriptRoot\runtime-helpers.ps1"
Start-Sleep -Seconds 12
& "$PSScriptRoot\ttr11-core.ps1"
$a=Get-Addon
Assert-Equal $a.physicalMags.active $false 'Addon ammunition hooks are off; upstream owns dropped ammo'
& "$PSScriptRoot\dialogue-name-layout.ps1"
Send-XR @('input clear','head pose 0 1.1 0 0 0 0')
Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('player.AddItem 00004353 1','sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 00004353')|Out-Null;Start-Sleep -Seconds 1
Assert-Equal (Get-Addon).weaponHand 1 'Native scoped rifle uses the selected right hand'
foreach($z in '-0.5','-0.16','-0.02'){
 Send-XR @("hand r to grip .032 .93 $z 0 55 0 600");Start-Sleep -Milliseconds 900
 Save-Eyes ("latest-scope-"+$z.Replace('.','p'))
}
$a=Get-Addon
Assert-Equal $a.scopeSurfaceFailures 0 'No scope surface restore errors'
$a|ConvertTo-Json -Depth 15|Set-Content "$script:Repo\work\ttr-e14567bc\runtime-final-state.json"
Write-Output 'PASS latest host: native weapon/aid/stim handoff, reload toggle, height, dialogue, scope; addon ammo disabled'
