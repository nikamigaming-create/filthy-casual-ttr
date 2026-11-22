. "$PSScriptRoot\magazine-helpers.ps1"
function Pull-PhysicalPart([uint32]$PartId,[switch]$NativeReload){
 $recoverBefore=(Get-Addon).physicalMags.recovered
 Send-XR @('grip l 0','hand r to grip 0.45 1.0 -0.15 -40 55 5 600','hand l to grip -0.25 0.9 -0.1 0 -35 0 650');Start-Sleep -Milliseconds 800
 for($aim=0;$aim -lt 3;$aim++){
  $a=Get-Addon;$angles=(& python.exe "$PSScriptRoot\magazine-pose.py" aim $PartId)|ConvertFrom-Json
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 450" -f $s.handL.pos[0],$s.handL.pos[1],$s.handL.pos[2],$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 550
 }
 Send-XR @('grip l 1');Start-Sleep -Milliseconds 280
 $a=Get-Addon;$a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-gravity-armed.json"
 Assert-Equal $a.gloves[0].candidate $PartId 'Native gravity glove selects the real magazine'
 $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json;$f=$a.gloves[0].forward
 $px=$s.handL.pos[0]-.30*$f[0];$py=$s.handL.pos[1]-.30*$f[1];$pz=$s.handL.pos[2]-.30*$f[2]
 Send-XR @(("hand l to grip {0:F5} {1:F5} {2:F5} {3:F3} {4:F3} {5:F3} 180" -f $px,$py,$pz,$angles[0],$angles[1],$angles[2]));Start-Sleep -Milliseconds 250
 $deadline=[DateTime]::UtcNow.AddSeconds(4)
 do{$a=Get-Addon;if($a.nativeGrabRef -eq $PartId -or ($NativeReload -and $a.physicalMags.recovered -gt $recoverBefore)){break};Start-Sleep -Milliseconds 150}while([DateTime]::UtcNow -lt $deadline)
 $a|ConvertTo-Json -Depth 12|Set-Content "$script:Repo\evidence\magazine-gravity-result.json"
 if($NativeReload){Assert-Equal ($a.physicalMags.recovered -gt $recoverBefore) $true 'Native gravity-glove catch enters reload hand'}else{Assert-Equal $a.nativeGrabRef $PartId 'Native gravity-glove flick pulls magazine into the hand'}
}
