try{& "$PSScriptRoot\bottle-handoff.ps1"}catch{Write-Output "BOTTLE FAIL: $_"}
& "$PSScriptRoot\vehicle-fire-probe.ps1"
