param([string]$Test='.\tests\magazine-lifecycle.ps1',[int]$MaxSeconds=110,[int]$SceneHeightCm=-999)
$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
if(Get-Process FalloutNV -ErrorAction SilentlyContinue){throw 'Game already running'}
Copy-Item -LiteralPath build/SeatedControls.dll -Destination 'D:\TTWVR\MO2\mods\Filthy Casual TTR - Magazine Probe\NVSE\Plugins\SeatedControls.dll' -Force
$launchPath=(Resolve-Path .\launch-simulator.cmd).Path
$launchBytes=[IO.File]::ReadAllBytes($launchPath)
try {
 if($SceneHeightCm -ne -999){
  $ini='D:\TTWVR\MO2\mods\Filthy Casual TTR\NVSE\Plugins\SeatedControls.ini'
  [IO.File]::WriteAllText($ini,([regex]::Replace([IO.File]::ReadAllText($ini),'(?m)^HeightOffsetCm=.*$',"HeightOffsetCm=$SceneHeightCm")))
 }
 [IO.File]::WriteAllText($launchPath,([IO.File]::ReadAllText($launchPath).Replace('setlocal','setlocal'+"`r`n"+'set "SC_TEST_PHYSICAL_MAGS=1"')))
 & .\tests\run-qa.ps1 -Test $Test -MaxSeconds $MaxSeconds -DesktopLaunch
} finally {
 [IO.File]::WriteAllBytes($launchPath,$launchBytes)
 Copy-Item -LiteralPath work/magazine-personal.ini -Destination 'D:\TTWVR\MO2\mods\Filthy Casual TTR\NVSE\Plugins\SeatedControls.ini' -Force
}
