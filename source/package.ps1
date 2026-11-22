param(
    [Parameter(Mandatory)][string]$Rc21Archive,
    [Parameter(Mandatory)][string]$OutputZip,
    [Parameter(Mandatory)][ValidatePattern('^1\.8\.0-beta\.4-rc\.\d+(-qa)?$')][string]$Version,
    [Parameter(Mandatory)][string]$VehicleDll,
    [Parameter(Mandatory)][string]$FlightControls
)

$ErrorActionPreference = 'Stop'
$arguments = @('-B', "$PSScriptRoot/tools/package_player_release.py", $Rc21Archive, $OutputZip, '--version', $Version,
               '--vehicle-dll', $VehicleDll, '--flight-controls', $FlightControls)
& python @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
