param([switch]$CheckWheel)
. "$PSScriptRoot/runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 0 0')
Send-Game 'load FC_Base_Clear_Access' | Out-Null
Start-Sleep -Seconds 7
Send-Game @('CloseAllMenus','player.SetWeaponOut 0') | Out-Null
Park-Hands

$roadster = 'FF002DC4'
$ground = Get-Native 'GetTerrainHeight -54000 -81000'
$z = $ground + 20
Get-Native ('Call (CompileScript "FCReadyPlace.txt") (StringToRef "{0}") (-54000) (-81000) ({1}) (180)' -f $roadster,$z) | Out-Null
Send-Game @('player.SetPos X -54200','player.SetPos Y -81000',"player.SetPos Z $($ground + 10)",'player.SetAngle Z 90') | Out-Null
Start-Sleep -Seconds 2

$boardedAfter = [DateTime]::UtcNow
Get-Native ('Call (CompileScript "FCVehicleTest.txt") "{0}" 1' -f $roadster) | Out-Null
function Read-Drive {
    $report = Get-Item -LiteralPath "$script:Session/arcade-drive.json" -ErrorAction SilentlyContinue
    if ($report -and $report.LastWriteTimeUtc -ge $boardedAfter) {
        try { Get-Content -LiteralPath $report.FullName -Raw | ConvertFrom-Json -ErrorAction Stop } catch { }
    }
}
$deadline = [DateTime]::UtcNow.AddSeconds(12)
do { Start-Sleep -Milliseconds 200; $drive = Read-Drive } until ($drive.wheelHits -ge 2 -or [DateTime]::UtcNow -ge $deadline)
Assert-Equal ($drive.wheelHits -ge 2) $true 'The stock Roadster found road support and acquired arcade handling'
if ($CheckWheel) {
    . "$PSScriptRoot/driver-rim-helpers.ps1"
    $wheel = Wheel
    $script:wheelYaw = [Math]::Atan2(-$wheel.center[0],-$wheel.center[2])*180/[Math]::PI
    Move-Rim 0
    Move-Rim 0
    Send-XR 'grip l .96'
    Start-Sleep -Milliseconds 350
    Assert-Equal ((Wheel).grabbed -join ',') '1,0' 'Left hand grabs the installed stock rim'
    Move-Rim 22 -LeftOnly
    Assert-Equal ([Math]::Abs((Wheel).steer) -gt .1) $true 'Turning the stock rim changes steering'
    Save-Eyes 'dependency-roadster-wheel-turn' | Out-Null
    Send-XR 'grip l 0'
    Start-Sleep -Milliseconds 350
    Assert-Equal ((Wheel).grabbed -join ',') '0,0' 'Releasing the rim frees both hands'
    Move-Rim 0
    Send-XR 'grip r .96'
    Start-Sleep -Milliseconds 350
    Assert-Equal ((Wheel).grabbed -join ',') '0,1' 'Right hand grabs the same rim independently'
    Move-Rim -22
    Assert-Equal ([Math]::Abs((Wheel).steer) -gt .1) $true 'The right hand can steer in the other direction'
    Send-XR 'grip r 0'
}
function Position {
    (Get-Content -LiteralPath "$script:Session/vehicle-state.json" -Raw | ConvertFrom-Json).position
}
$start = Position
Send-XR 'trigger r 1'
Start-Sleep -Seconds 3
$moving = Read-Drive
$end = Position
Assert-Equal ($moving.speedMps -gt 2) $true 'Stock Roadster accelerates from the right trigger'
Assert-Equal ([Math]::Abs($end[0]-$start[0])+[Math]::Abs($end[1]-$start[1]) -gt 60) $true 'The native car moved in the world'
Send-XR @('trigger r 0','btn b press 200')
Start-Sleep -Seconds 2
Assert-Equal (Get-Addon).seatContext 0 'B restores the on-foot context'
