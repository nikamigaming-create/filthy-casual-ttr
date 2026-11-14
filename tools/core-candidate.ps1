param([ValidateSet('Stage','Verify','Install')][string]$Action='Verify',[Parameter(Mandatory)][string]$Candidate)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$Candidate=[IO.Path]::GetFullPath($Candidate)
$mod='D:\TTWVR\MO2\mods\Filthy Casual TTR'
function Check-Payload {
 $m=Get-Content (Join-Path $Candidate 'manifest.json') -Raw|ConvertFrom-Json
 foreach($f in $m.files){
  $path=[IO.Path]::GetFullPath((Join-Path (Join-Path $Candidate 'payload') $f.path))
  if(!$path.StartsWith([IO.Path]::GetFullPath((Join-Path $Candidate 'payload'))+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Invalid manifest path'}
  if((Get-FileHash -LiteralPath $path).Hash -ne $f.sha256){throw "Candidate changed: $($f.path)"}
 }
 return $m
}
if($Action -eq 'Stage'){
 if(Test-Path -LiteralPath $Candidate){throw 'Use a new candidate directory; never overwrite tested payloads'}
 $payload=Join-Path $Candidate 'payload';New-Item -ItemType Directory -Path $payload|Out-Null
 $pairs=@(@('build\SeatedControls.dll','NVSE\Plugins\SeatedControls.dll'),@('build\SeatedInput.dll','NVSE\Plugins\SeatedControls\SeatedInput.dll'),@('SeatedInput.json','NVSE\Plugins\SeatedControls\SeatedInput.json'))
 foreach($file in Get-ChildItem (Join-Path $repo 'build\glass-assets') -Recurse -File){$pairs+=,@($file.FullName,$file.FullName.Substring((Join-Path $repo 'build\glass-assets').Length+1))}
 foreach($pair in $pairs){$src=if([IO.Path]::IsPathRooted($pair[0])){$pair[0]}else{Join-Path $repo $pair[0]};$dst=Join-Path $payload $pair[1];New-Item -ItemType Directory -Force (Split-Path $dst)|Out-Null;Copy-Item -LiteralPath $src -Destination $dst}
 $files=@(Get-ChildItem $payload -Recurse -File|ForEach-Object {@{path=$_.FullName.Substring($payload.Length+1);sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}})
 @{createdUtc=[DateTime]::UtcNow.ToString('o');files=$files}|ConvertTo-Json -Depth 5|Set-Content (Join-Path $Candidate 'manifest.json')
}
$manifest=Check-Payload
if($Action -eq 'Install'){
 if(Get-Process FalloutNV -ErrorAction SilentlyContinue){throw 'Game running; refusing installation'}
 $proof=Get-Content (Join-Path $Candidate 'verification.json') -Raw|ConvertFrom-Json
 if(!$proof.passed -or !$proof.visualReviewed -or $proof.manifestSha256 -ne (Get-FileHash (Join-Path $Candidate 'manifest.json')).Hash){throw 'No passing check for this exact candidate'}
 $backup=Join-Path 'D:\TTWVR\Backups' ('Core-Candidate-'+(Get-Date -Format yyyyMMdd-HHmmss));New-Item -ItemType Directory -Path $backup|Out-Null
 $ini=Join-Path $mod 'NVSE\Plugins\SeatedControls.ini';$iniHash=(Get-FileHash -LiteralPath $ini).Hash
 $copied=@()
 try{
  foreach($f in $manifest.files){$dst=Join-Path $mod $f.path;$old=Join-Path $backup $f.path;$existed=Test-Path -LiteralPath $dst;if($existed){New-Item -ItemType Directory -Force (Split-Path $old)|Out-Null;Copy-Item -LiteralPath $dst -Destination $old};$copied+=@{path=$f.path;existed=$existed};New-Item -ItemType Directory -Force (Split-Path $dst)|Out-Null;Copy-Item -LiteralPath (Join-Path (Join-Path $Candidate 'payload') $f.path) -Destination $dst -Force;if((Get-FileHash -LiteralPath $dst).Hash -ne $f.sha256){throw "Installed hash mismatch: $($f.path)"}}
  if((Get-FileHash -LiteralPath $ini).Hash -ne $iniHash){throw 'Personal settings changed'}
 }catch{foreach($f in $copied){$dst=Join-Path $mod $f.path;if($f.existed){Copy-Item -LiteralPath (Join-Path $backup $f.path) -Destination $dst -Force}else{Remove-Item -LiteralPath $dst}};throw}
 @{candidate=$Candidate;backup=$backup;files=$manifest.files;iniSha256=$iniHash;installedUtc=[DateTime]::UtcNow.ToString('o')}|ConvertTo-Json -Depth 6|Set-Content (Join-Path $Candidate 'installed.json')
 Write-Output "Installed $($manifest.files.Count) matching files; personal INI preserved; backup $backup"
}else{Write-Output "Verified $($manifest.files.Count) candidate files"}
