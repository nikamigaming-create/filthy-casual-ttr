param()
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$ini='D:\TTWVR\MO2\mods\Filthy Casual TTR\NVSE\Plugins\SeatedControls.ini'
if(Get-Process FalloutNV -ErrorAction SilentlyContinue){throw 'Close the owned pool test before starting the feature capture'}
$original=[IO.File]::ReadAllBytes($ini)
$before=(Get-FileHash -LiteralPath $ini).Hash
[IO.File]::WriteAllBytes((Join-Path $repo 'work\mobile-capture-original.ini'),$original)
try {
 $text=[IO.File]::ReadAllText($ini)
 $text=[regex]::Replace($text,'(?m)^HeightOffsetCm=.*$','HeightOffsetCm=0')
 $text=[regex]::Replace($text,'(?m)^AutomaticReload=.*$','AutomaticReload=1')
 [IO.File]::WriteAllText($ini,$text)
 & "$PSScriptRoot\run-qa.ps1" -Test "$PSScriptRoot\showcase-human.ps1" -MaxSeconds 260 -DesktopLaunch
 & "$PSScriptRoot\run-qa.ps1" -Test "$PSScriptRoot\showcase-pickups.ps1" -MaxSeconds 180 -DesktopLaunch
} finally {
 [IO.File]::WriteAllBytes($ini,$original)
 $after=(Get-FileHash -LiteralPath $ini).Hash
 if($after -ne $before){throw 'Personal seated settings were not restored exactly'}
 Write-Output "Personal seated settings restored byte-for-byte: $after"
}
