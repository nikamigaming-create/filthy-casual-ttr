$ErrorActionPreference='Stop'
$results=[Collections.Generic.List[object]]::new()
foreach($case in @(@{Weapon=0x434F;Rounds=0;SaveReload=$true},@{Weapon=0x8F21E;Rounds=13},@{Weapon=0x90727;Rounds=8;Gravity=$true})){
 try{& "$PSScriptRoot\magazine-reinsert.ps1" @case;$results.Add(@{case=$case;pass=$true})}
 catch{$results.Add(@{case=$case;pass=$false;error=$_.ToString();stack=$_.ScriptStackTrace});Write-Warning $_}
 $results|ConvertTo-Json -Depth 6|Set-Content "$PSScriptRoot\..\evidence\magazine-reinsertion-matrix.json"
}
if(@($results|Where-Object {!$_.pass}).Count){throw 'Recovery matrix has recorded failures'}
