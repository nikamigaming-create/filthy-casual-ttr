. "$PSScriptRoot\casual-guide-helpers.ps1"
Load-Guide
Send-Game @('player.damageav health 50') | Out-Null
Choose-Aid 86377
$doses=Get-Native 'player.GetItemCount Stimpak'
Send-XR @('trigger r 1')
foreach($y in .67,.85,1.0,1.22){
    Send-XR @("hand r grip pose 0.10 $y -0.50 0 0 0")
    Start-Sleep -Milliseconds 300
    Send-XR @("hand r to grip 0.02 $y 0.03 0 0 0 200")
    Start-Sleep -Milliseconds 450
    $after=Get-Native 'player.GetItemCount Stimpak'
    Write-Output "Native Stimpak seated head=1.1, handY=$y, rotation=0: doses $after"
    if($after -lt $doses){Save-Eyes 'casual-stim-success';break}
}
Send-XR @('trigger r 0')
Assert-Equal $after ($doses-1) 'A reachable seated native body-jab pose consumes one dose'
