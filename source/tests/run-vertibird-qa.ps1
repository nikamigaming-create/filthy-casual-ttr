param([string]$Test="$PSScriptRoot\vertibird-smoke.ps1",[int]$MaxSeconds=140)
$ErrorActionPreference='Stop'
if(Get-Process FalloutNV -ErrorAction SilentlyContinue){throw 'A game is running'}
$profile='D:\TTWVR\MO2\profiles\TTWVR - Addon QA'
$backup=Join-Path (Split-Path $PSScriptRoot -Parent) 'work\vertibird\profile-backup'
New-Item -ItemType Directory -Force $backup|Out-Null
foreach($n in 'modlist.txt','plugins.txt','loadorder.txt'){Copy-Item -LiteralPath "$profile\$n" -Destination "$backup\$n" -Force}
try{
 $mods=@('Filthy Casual Vehicles - Development','TTWVR - Vertibird Development','Vertibird Player Home - Pilot Inside','Vertibird Player Home TTW Patch 4.18c','Vertibird Player Home 4.18')
 $p="$profile\modlist.txt";$lines=@(Get-Content -LiteralPath $p | Where-Object {$_.Length -lt 2 -or $_.Substring(1) -notin $mods})
 [IO.File]::WriteAllLines($p,@($mods | ForEach-Object {'+'+$_})+$lines)
 foreach($n in 'plugins.txt','loadorder.txt'){
  $p="$profile\$n";$lines=@(Get-Content -LiteralPath $p | Where-Object {$_ -notin @('Player Vertibird.esm','Player Vertibird TTW Patch.esp','Vertibird Player Home - pilot inside.esp')})
  $last=0;for($i=0;$i -lt $lines.Count;$i++){if($lines[$i] -like '*.esm'){$last=$i}}
  $next=@($lines[0..$last])+@('Player Vertibird.esm')+@($lines[($last+1)..($lines.Count-1)])+@('Player Vertibird TTW Patch.esp','Vertibird Player Home - pilot inside.esp')
  [IO.File]::WriteAllLines($p,$next)
 }
 & "$PSScriptRoot\run-qa.ps1" -Test $Test -MaxSeconds $MaxSeconds -DesktopLaunch
}finally{foreach($n in 'modlist.txt','plugins.txt','loadorder.txt'){Copy-Item -LiteralPath "$backup\$n" -Destination "$profile\$n" -Force}}
