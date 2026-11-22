param([string]$Test="$PSScriptRoot\vehicle-smoke.ps1",[int]$MaxSeconds=140)
$ErrorActionPreference='Stop'
if(Get-Process FalloutNV -ErrorAction SilentlyContinue){throw 'A game is running'}
$profile='D:\TTWVR\MO2\profiles\TTWVR - Addon QA'
$backup=Join-Path (Split-Path $PSScriptRoot -Parent) 'work\vehicles\profile-backup'
New-Item -ItemType Directory -Force $backup|Out-Null
foreach($n in 'modlist.txt','plugins.txt','loadorder.txt'){Copy-Item -LiteralPath "$profile\$n" -Destination "$backup\$n" -Force}
try{
 $p="$profile\modlist.txt";$s=[IO.File]::ReadAllText($p);$s=[regex]::Replace($s,'(?m)^[+-]Uhmatt Drivable Cars 8b\r?\n','');[IO.File]::WriteAllText($p,"+Filthy Casual Vehicles - Development`r`n+TTWVR - Vertibird Development`r`n+Uhmatt Drivable Cars 8b`r`n"+$s)
 foreach($n in 'plugins.txt','loadorder.txt'){$p="$profile\$n";$s=[IO.File]::ReadAllText($p);if(!$s.Contains('UmDrivingVIIIb.esp')){[IO.File]::WriteAllText($p,$s.TrimEnd()+"`r`nUmDrivingVIIIb.esp`r`n")}}
 & "$PSScriptRoot\run-qa.ps1" -Test $Test -MaxSeconds $MaxSeconds -DesktopLaunch
}finally{foreach($n in 'modlist.txt','plugins.txt','loadorder.txt'){Copy-Item -LiteralPath "$backup\$n" -Destination "$profile\$n" -Force}}
