param([Parameter(Mandatory)][string]$Candidate)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$Candidate=[IO.Path]::GetFullPath($Candidate)
Push-Location $repo
try{
 if(Get-Process FalloutNV -ErrorAction SilentlyContinue){throw 'Game running; save and quit first'}
 & .\tests\check-sticky-food.cmd;if($LASTEXITCODE){throw 'Mouth unit checks failed'}
 & .\build.cmd;if($LASTEXITCODE){throw 'Core/input build failed'}
 & .\tools\core-candidate.ps1 -Action Stage -Candidate $Candidate
 & .\tests\run-controls-vehicles-qa.ps1 -Test tests\food-glass-probe.ps1 -MaxSeconds 240 -CoreCandidate $Candidate -SettingsPath 'D:\TTWVR\MO2\mods\Filthy Casual TTR\NVSE\Plugins\SeatedControls.ini' *> (Join-Path $Candidate 'runtime.log')
 & .\tools\core-candidate.ps1 -Action Verify -Candidate $Candidate
 $log=Get-Content (Join-Path $Candidate 'runtime.log') -Raw
 foreach($id in '001613C5','0001519B','00015163','00032C75'){if($log -notmatch "PASS: $id consumed exactly once"){throw "Missing consumption check $id"}}
 foreach($surface in 'reported-desert','original-floor'){if($log -notmatch "PASS: $surface detected native impact"){throw "Missing impact check $surface"}}
 Copy-Item 'D:\SteamLibrary\steamapps\common\Fallout New Vegas\SeatedControls.log' (Join-Path $Candidate 'native.log')
 $native=Get-Content (Join-Path $Candidate 'native.log') -Raw
 if(([regex]::Matches($native,'GLASS NATIVE tick=1 .*init=1.*verts=[1-9].*tris=[1-9]')).Count -lt 2){throw 'Missing actual projected glass geometry'}
 @{passed=$true;visualReviewed=$false;manifestSha256=(Get-FileHash (Join-Path $Candidate 'manifest.json')).Hash;runtime='food-glass-probe.ps1';note='Review both-eye 2s/4s/12s captures before setting visualReviewed and installing'}|ConvertTo-Json|Set-Content (Join-Path $Candidate 'verification.json')
 Write-Output 'Runtime checks passed. Candidate remains uninstalled until both-eye visual review.'
}finally{Pop-Location}
