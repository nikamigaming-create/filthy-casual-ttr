$ErrorActionPreference='Stop'
Start-Sleep -Seconds 12
& "$PSScriptRoot\ttr115-final.ps1"
& "$PSScriptRoot\bottle-handoff.ps1"
Write-Output 'PASS near-wall replacement and four bottle families, native release and visible physics'
