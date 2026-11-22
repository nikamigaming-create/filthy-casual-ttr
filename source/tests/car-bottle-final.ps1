$ErrorActionPreference='Stop'
$failures=@()
foreach($test in 'bottle-handoff.ps1','vehicle-contact-motion.ps1'){
 try{& "$PSScriptRoot\$test"}catch{$failures+="$test : $_";Write-Output "FAIL: $test : $_"}
}
if($failures.Count){throw ($failures -join '; ')}
