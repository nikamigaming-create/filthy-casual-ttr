param([string]$OriginalModPath, [string]$Destination = $PSScriptRoot)
$ErrorActionPreference='Stop'
if(!$OriginalModPath){
 Add-Type -AssemblyName System.Windows.Forms
 $picker=New-Object System.Windows.Forms.FolderBrowserDialog
 $picker.Description='Choose the ORIGINAL Pool Tables Of The Mojave mod folder (the folder containing Pool Table Minigame.esp).'
 if($picker.ShowDialog() -ne 'OK'){throw 'No original mod folder selected. Nothing changed.'}
 $OriginalModPath=$picker.SelectedPath
}
$source=(Resolve-Path -LiteralPath $OriginalModPath).Path
if(!(Test-Path -LiteralPath (Join-Path $source 'Pool Table Minigame.esp'))){throw 'Select the original pool mod folder containing Pool Table Minigame.esp.'}
$sourceScripts=Join-Path $source 'NVSE\user_defined_functions\Minigames\Pool'
$setup=Join-Path $PSScriptRoot 'setup'
$recipe=Get-Content -LiteralPath (Join-Path $setup 'recipe.json') -Raw|ConvertFrom-Json
function File-Digest([string]$Path){$sha=[Security.Cryptography.SHA256]::Create();$stream=[IO.File]::OpenRead($Path);try{return ([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','')}finally{$stream.Dispose();$sha.Dispose()}}
$inputText=@{}
foreach($item in $recipe.inputs.PSObject.Properties){
 $path=Join-Path $sourceScripts $item.Name
 if(!(Test-Path -LiteralPath $path)){throw "Missing original script: $($item.Name). Install Pool Tables Of The Mojave v4 separately."}
 if((File-Digest $path) -ne $item.Value){throw "Unsupported original script: $($item.Name). This patch supports the verified v4 download. Original files have not been changed."}
 $inputText[$item.Name]=[IO.File]::ReadAllText($path).Replace("`r`n","`n")
}
function Segment([string]$Text,[int]$Start,[int]$Count){$lines=$Text.Split([char]10);return (($lines[$Start..($Start+$Count-1)] -join "`n")+"`n")}
$defaults=($inputText['Open.txt'] -split '; BEGIN STATE DEFAULTS',2)[1] -split '; END STATE DEFAULTS',2
$start=[IO.File]::ReadAllText((Join-Path $setup 'Start.template')).Replace("`r`n","`n").Replace('@@UPSTREAM_DEFAULTS@@',$defaults[0])
$physics=Segment $inputText['Tick.txt'] $recipe.physicsRange[0] $recipe.physicsRange[1]
foreach($pair in $recipe.physicsReplacements){$physics=$physics.Replace($pair[0],$pair[1])}
$shot=Segment $inputText['MatchFrame.txt'] $recipe.shotRange[0] $recipe.shotRange[1]
$shot=$shot.Replace('aCue','cue');$shot=[regex]::Replace($shot,'(?m)^(?=.)','        ')
$tick=[IO.File]::ReadAllText((Join-Path $setup 'Tick.template')).Replace("`r`n","`n").Replace("@@UPSTREAM_PHYSICS@@`n",$physics).Replace("@@UPSTREAM_SHOT@@`n",$shot)
$insert=[IO.File]::ReadAllText((Join-Path $setup 'CandidateInsertion.txt')).Replace("`r`n","`n")
$candidate=$inputText['PlanCandidate.txt'].Replace($recipe.candidateAnchor,$insert+$recipe.candidateAnchor)
$results=@{'Start.txt'=$start;'Tick.txt'=$tick;'PlanCandidate.txt'=$candidate}
$utf8=[Text.UTF8Encoding]::new($false)
foreach($name in $results.Keys){$sha=[Security.Cryptography.SHA256]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash($utf8.GetBytes($results[$name])))).Replace('-','')}finally{$sha.Dispose()};if($hash -ne $recipe.expected.$name){throw "Generated $name differs from the tested integration. No output written."}}
$dest=[IO.Path]::GetFullPath($Destination)
if($dest.TrimEnd('\') -eq $source.TrimEnd('\')){throw 'Destination must be our addon folder, not the original mod.'}
$target=Join-Path $dest 'NVSE\user_defined_functions\FilthyCasualPool'
New-Item -ItemType Directory -Path $target -Force|Out-Null
foreach($name in $results.Keys){[IO.File]::WriteAllText((Join-Path $target $name),$results[$name],$utf8)}
Write-Output "Prepared all three integration scripts in $target. Original mod untouched. Enable Filthy Casual Pool below the original mod in MO2."
