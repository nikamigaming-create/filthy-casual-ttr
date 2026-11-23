. "$PSScriptRoot/runtime-helpers.ps1"
Assert-Equal (Get-Native 'IsModLoaded "Player Vertibird.esm"') 0 'Aircraft master is absent'
Assert-Equal (Get-Native 'IsModLoaded "Player Vertibird TTW Patch.esp"') 0 'Aircraft TTW patch is absent'
Assert-Equal (Get-Native 'IsModLoaded "Vertibird Player Home - pilot inside.esp"') 0 'Pilot Inside is absent'
Assert-Equal (Get-Native 'IsModLoaded "UmDrivingVIIIb.esp"') 1 'Uhmatt cars are active'
& "$PSScriptRoot/dependency-only-roadster.ps1" -CheckWheel
