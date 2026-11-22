$ErrorActionPreference='Stop'
& "$PSScriptRoot\ttr11-core.ps1"
& "$PSScriptRoot\magazine-lifecycle.ps1"
& "$PSScriptRoot\ttr11-presentation-pool.ps1"
Write-Output 'PASS combined TTR1.1.2 core, physical magazine, dialogue, pool and scope run'
