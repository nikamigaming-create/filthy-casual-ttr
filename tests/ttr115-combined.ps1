$ErrorActionPreference='Stop'
. "$PSScriptRoot\runtime-helpers.ps1"
Start-Sleep -Seconds 12
& "$PSScriptRoot\release-native-checks.ps1"
& "$PSScriptRoot\controls-runtime.ps1"
& "$PSScriptRoot\seat-mouth-ride-ttr115.ps1"
Write-Output 'PASS TTR1.1.5 combined magazines, food, glass, input editor, seat contexts, driving, flight and creature ride'
