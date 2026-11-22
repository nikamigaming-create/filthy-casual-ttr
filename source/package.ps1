param(
    [Parameter(Mandatory)][string]$Rc21Archive,
    [Parameter(Mandatory)][string]$OutputZip,
    [Parameter(Mandatory)][ValidatePattern('^1\.8\.0-beta\.4-rc\.\d+(-qa)?$')][string]$Version,
    [string]$VehicleDll
)

$ErrorActionPreference = 'Stop'
$arguments = @('-B', "$PSScriptRoot/tools/package_player_release.py", $Rc21Archive, $OutputZip, '--version', $Version)
if ($VehicleDll) { $arguments += @('--vehicle-dll', $VehicleDll) }
& python @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
