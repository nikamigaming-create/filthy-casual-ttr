. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 6
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
$actor=Get-Native 'GetFirstRef 43'
for($i=0;$i -lt 10 -and $actor -eq 0x14;$i++){$actor=Get-Native 'GetNextRef'}
if(!$actor){throw 'No native NPC in the fixture'}
$ref='{0:X8}' -f [int]$actor
Write-Output "Dialogue NPC reference $ref"
Send-Game @("player.MoveTo $ref 0 70 0",("expr Call ({{ref actor}} => actor.StartConversation player) (StringToRef `"$ref`")"))|Out-Null
Start-Sleep -Seconds 4
$a=Get-Addon
Assert-Equal ($a.dialoguePlacements -gt 0) $true 'Native dialogue positioning hook executed'
Assert-Equal ($a.dialogueSideDegrees -ge 30) $true 'Dialogue has side clearance'
Save-Eyes 'dialogue-clearance-near'
Send-XR @('head to 0.04 1.1 0 35 0 0 700','hand r to grip 0.3 0.8 -0.3 20 55 10 700')
Start-Sleep -Seconds 1
Save-Eyes 'dialogue-clearance-turn'
