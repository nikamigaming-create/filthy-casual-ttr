param([string]$Test="$PSScriptRoot\controls-vehicles.ps1",[int]$MaxSeconds=420,[switch]$Recovery,[string]$CoreCandidate,[string]$SettingsPath)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent;$mods='D:\TTWVR\MO2\mods';$stage="$mods\TTWVR - RobCo Vehicle Test"
if(Get-Process FalloutNV -ErrorAction SilentlyContinue){throw 'Game already running'}
foreach($pair in @(@('build\SeatedControls.dll','NVSE\Plugins\SeatedControls.dll'),@('build\SeatedInput.dll','NVSE\Plugins\SeatedControls\SeatedInput.dll'),@('SeatedInput.json','NVSE\Plugins\SeatedControls\SeatedInput.json'))){$target=Join-Path $stage $pair[1];New-Item -ItemType Directory -Force (Split-Path $target)|Out-Null;Copy-Item -LiteralPath (Join-Path $repo $pair[0]) -Destination $target -Force}
Copy-Item -LiteralPath "$mods\TTWVR Latest - Compatibility Test\NVSE\Plugins\SeatedControls.ini" -Destination "$stage\NVSE\Plugins\SeatedControls.ini" -Force
$ini=[IO.File]::ReadAllText("$stage\NVSE\Plugins\SeatedControls.ini");$ini=$ini.Replace('ShowSelectorPopups=0','ShowSelectorPopups=1');[IO.File]::WriteAllText("$stage\NVSE\Plugins\SeatedControls.ini",$ini)
if($Recovery){Add-Content -LiteralPath "$stage\NVSE\Plugins\SeatedControls.ini" -Value "`r`n[Reload]`r`nRecoverWorldMagazines=1`r`n"}
$assets="$stage\NVSE\Plugins\SeatedControls\Controls";New-Item -ItemType Directory -Force $assets|Out-Null;Copy-Item -Path "$repo\assets\controls\*" -Destination $assets -Force -Recurse
Copy-Item -Path "$repo\build\tutorial-assets\meshes","$repo\build\tutorial-assets\textures" -Destination $stage -Recurse -Force
Copy-Item -Path "$repo\build\glass-assets\*" -Destination $stage -Recurse -Force
if($CoreCandidate){
 & "$repo\tools\core-candidate.ps1" -Action Verify -Candidate $CoreCandidate
 Copy-Item -Path "$CoreCandidate\payload\*" -Destination $stage -Recurse -Force
}
if($SettingsPath){Copy-Item -LiteralPath $SettingsPath -Destination "$stage\NVSE\Plugins\SeatedControls.ini" -Force}
$helper="$stage\NVSE\user_defined_functions";New-Item -ItemType Directory -Force $helper|Out-Null
Copy-Item -LiteralPath "$repo\tests\fixtures\FCVehicleReverse.txt" -Destination $helper -Force
Copy-Item -LiteralPath "$repo\tests\fixtures\FCRideQuery.txt" -Destination $helper -Force
Copy-Item -LiteralPath "$repo\tests\fixtures\FCStageParked.txt" -Destination $helper -Force
Copy-Item -LiteralPath "$repo\tests\fixtures\FCBottleImpact.txt" -Destination $helper -Force
$vehicle="$mods\Filthy Casual Vehicles - Development\NVSE\Plugins"
Copy-Item -LiteralPath "$repo\build\vehicle-vr\FilthyCasualVehicles.dll" -Destination $vehicle -Force
Copy-Item -LiteralPath "$repo\build\vehicle-vr\FilthyCasualVehiclesInput.dll" -Destination "$vehicle\FilthyCasualVehicles\FilthyCasualVehiclesInput.dll" -Force
$profile='D:\TTWVR\MO2\profiles\TTWVR - Addon QA';$backup="$repo\work\controls-vehicles\backup";New-Item -ItemType Directory -Force $backup|Out-Null
foreach($n in 'modlist.txt','plugins.txt','loadorder.txt'){Copy-Item -LiteralPath "$profile\$n" -Destination "$backup\$n" -Force}
$oldCaptureAddon=$env:FC_CAPTURE_ADDON_PATH
$env:FC_CAPTURE_ADDON_PATH="$stage\NVSE\Plugins\SeatedControls.dll"
try{
 $enable=@('TTWVR - RobCo Vehicle Test','Filthy Casual Vehicles - Development','TTWVR - Vertibird Development','Vertibird Player Home - Pilot Inside','Vertibird Player Home TTW Patch 4.18c','Vertibird Player Home 4.18','Uhmatt Drivable Cars 8b')
 $disable=@('TTWVR Latest - Compatibility Test','Filthy Casual Pool','Filthy Casual Pool - Equipment Development','Pool Tables Of The Mojave')
 $lines=@(Get-Content "$profile\modlist.txt"|Where-Object {$_.Length -lt 2 -or $_.Substring(1) -notin ($enable+$disable)})
 [IO.File]::WriteAllLines("$profile\modlist.txt",@($enable|ForEach-Object {'+'+$_})+@($disable|ForEach-Object {'-'+$_})+$lines)
 foreach($n in 'plugins.txt','loadorder.txt'){$p="$profile\$n";$lines=@(Get-Content $p|Where-Object {$_ -notin @('Player Vertibird.esm','Player Vertibird TTW Patch.esp','Vertibird Player Home - pilot inside.esp','UmDrivingVIIIb.esp','Filthy Casual Pool.esp','Filthy Casual Pool Equipment.esp','Pool Table Minigame.esp','Filthy Casual Glass.esp')});$last=0;for($i=0;$i -lt $lines.Count;$i++){if($lines[$i] -like '*.esm'){$last=$i}};[IO.File]::WriteAllLines($p,@($lines[0..$last])+@('Player Vertibird.esm')+@($lines[($last+1)..($lines.Count-1)])+@('UmDrivingVIIIb.esp','Player Vertibird TTW Patch.esp','Vertibird Player Home - pilot inside.esp','Filthy Casual Glass.esp'))}
 & "$repo\tests\run-qa.ps1" -Test $Test -MaxSeconds $MaxSeconds -DesktopLaunch
}finally{$env:FC_CAPTURE_ADDON_PATH=$oldCaptureAddon;foreach($n in 'modlist.txt','plugins.txt','loadorder.txt'){Copy-Item -LiteralPath "$backup\$n" -Destination "$profile\$n" -Force}}
