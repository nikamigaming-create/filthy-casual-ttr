$ErrorActionPreference='Stop'
. "$PSScriptRoot\runtime-helpers.ps1"
Start-Sleep -Seconds 12
$global:FCSeatAcceptanceGround=(Get-Addon).groundHeightOffset
function Mode-Check([string]$name,[string]$capture){
 $mode=if($name -eq 'vehicles.driving'){1}else{2};$delta=if($mode -eq 1){.05}else{-.05}
 $a=Get-Addon;Assert-Equal $a.seatContext $mode 'Seat settings follow actual native mounted mode'
 Send-Game @('sc_settings')|Out-Null;Start-Sleep -Milliseconds 350
 Send-XR @('btn x press 120');Start-Sleep -Milliseconds 200
 Send-XR @(("stick r 0 {0}" -f [Math]::Sign($delta)));Start-Sleep -Milliseconds 120;Send-XR @('stick r center');Start-Sleep -Milliseconds 200
 Assert-Equal ([Math]::Abs((Get-Addon).heightOffset-$delta) -lt .001) $true 'Only this seat height adjusted'
 Save-Eyes ("seat-context-"+$mode)
 Send-XR @('btn b press 150');Start-Sleep -Milliseconds 350
 Assert-Equal (Get-Addon).seatContext $mode 'Saving height does not dismount'
 Assert-Equal ([Math]::Abs((Get-Addon).groundHeightOffset-$global:FCSeatAcceptanceGround) -lt .001) $true 'Mounted setting preserves ground calibration'
}
& "$PSScriptRoot\vertibird-smoke.ps1"
Assert-Equal (Get-Addon).seatContext 0 'Flight exit restores ground context'
Assert-Equal ([Math]::Abs((Get-Addon).heightOffset-$global:FCSeatAcceptanceGround) -lt .001) $true 'Flight exit preserves ground calibration'
$ini=Get-Content 'D:\TTWVR\MO2\mods\TTWVR 1.1.5 - Compatibility Test\NVSE\Plugins\SeatedControls.ini' -Raw
Assert-Equal ($ini -match '(?m)^FlightSeatOffsetCm=-5\.0') $true 'Flight offset persisted separately'
& "$PSScriptRoot\bighorner-challenge-video.ps1"
& "$PSScriptRoot\ttr115-final.ps1"
Write-Output 'PASS final 1.1.5 flight, ride, dialogue and bottle validation'
