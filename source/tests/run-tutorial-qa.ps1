param([string]$Test="$PSScriptRoot\tutorial-native-foundation.ps1",[int]$MaxSeconds=210,[switch]$CurrentInput)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process FalloutNV -ErrorAction SilentlyContinue){throw 'A game is running'}
$backup="$repo\work\tutorial-dev\profile-backup"
New-Item -ItemType Directory -Force -Path $backup|Out-Null
$profile='D:\TTWVR\MO2\profiles\TTWVR - Addon QA'
foreach($name in 'modlist.txt','plugins.txt','loadorder.txt'){Copy-Item -LiteralPath "$profile\$name" -Destination "$backup\$name" -Force}
$mod='D:\TTWVR\MO2\mods\TTWVR - Tutorial Development'
$plugins="$mod\NVSE\Plugins"
New-Item -ItemType Directory -Force -Path "$plugins\SeatedControls"|Out-Null
Copy-Item "$repo\build\SeatedControls.dll" "$plugins\SeatedControls.dll" -Force
Copy-Item 'D:\TTWVR\MO2\mods\Filthy Casual TTR\NVSE\Plugins\SeatedControls.ini' "$plugins\SeatedControls.ini" -Force
Copy-Item 'D:\TTWVR\MO2\mods\Filthy Casual TTR\NVSE\Plugins\SeatedControls\SeatedInput.*' "$plugins\SeatedControls" -Force
if($CurrentInput){Copy-Item "$repo\build\SeatedInput.dll" "$plugins\SeatedControls\SeatedInput.dll" -Force}
foreach($dir in 'meshes','textures'){Copy-Item -LiteralPath "$repo\build\tutorial-assets\$dir" -Destination $mod -Recurse -Force}
$text=[IO.File]::ReadAllText("$profile\modlist.txt")
$text=[regex]::Replace($text,'(?m)^[+-]TTWVR - Tutorial Development\r?\n','')
[IO.File]::WriteAllText("$profile\modlist.txt","+TTWVR - Tutorial Development`r`n"+$text)
foreach($name in 'plugins.txt','loadorder.txt'){
 $p="$profile\$name";$text=[IO.File]::ReadAllText($p)
 if(!$text.Contains('Filthy Casual Reload Parts.esp')){[IO.File]::WriteAllText($p,$text.TrimEnd()+"`r`nFilthy Casual Reload Parts.esp`r`n")}
}
try {& "$PSScriptRoot\run-qa.ps1" -Test $Test -MaxSeconds $MaxSeconds -DesktopLaunch}
finally {foreach($name in 'modlist.txt','plugins.txt','loadorder.txt'){Copy-Item -LiteralPath "$backup\$name" -Destination "$profile\$name" -Force}}
