. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('head to 0 1.1 0 0 -20 0 800','hand l grip pose -0.4 0.65 -0.1 0 0 0')
$samples=@();$i=0
foreach($angles in @('0 0 0','0 55 0','0 0 90','0 0 -90','60 0 0','-60 0 0')){
 Send-XR @("hand r to grip 0.1 0.95 -0.35 $angles 900")
 Start-Sleep -Milliseconds 1200
 $a=Get-Addon;$samples+=@{index=$i;angles=$angles;wrist=$a.wrist}
 Write-Output "Pose $i : $angles : attached=$($a.wrist.attached) valid=$($a.wrist.valid) facing=$($a.wrist.facing)"
 Save-Eyes "wrist-fit-$i"
 $i++
}
$samples|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\wrist-fit.json"
Assert-Equal (@($samples|Where-Object {$_.wrist.attached}).Count -gt 0) $true 'Original NIF attached to native wrist'
Assert-Equal (@($samples|Where-Object {$_.wrist.facing}).Count -gt 0) $true 'Anatomical wrist can face the seated viewer'
