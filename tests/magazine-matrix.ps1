. "$PSScriptRoot\runtime-helpers.ps1"
$results=[Collections.Generic.List[object]]::new()
foreach($case in @(@{id=0x0008F21E;name='carbine'},@{id=0x00090727;name='plasma-defender'},@{id=0x00004335;name='laser-pistol'},@{id=0x0008F216;name='revolver'},@{id=0x000E393B;name='shotgun'})) {
 try {
  & "$PSScriptRoot\magazine-lifecycle.ps1" -WeaponId $case.id -Label $case.name
  $results.Add(@{weapon=$case.name;form=$case.id;pass=$true})
 } catch {
  $results.Add(@{weapon=$case.name;form=$case.id;pass=$false;error=$_.ToString()})
  Write-Warning ($case.name+': '+$_)
 }
 $results|ConvertTo-Json -Depth 6|Set-Content "$script:Repo\evidence\magazine-matrix-final.json"
}
if(@($results|Where-Object {!$_.pass}).Count){throw 'Weapon permutation failures recorded in magazine-matrix.json'}
