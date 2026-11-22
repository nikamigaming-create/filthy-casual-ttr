. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Send-Game @('player.AddItem 0001519B 5','player.damageav health 80')|Out-Null
function Pick-Food {
 Send-XR @('input clear');Park-Hands
 Send-Game @('sc_aid')|Out-Null;Start-Sleep -Milliseconds 350
 for($page=0;$page -lt 5;$page++){
  $a=Get-Addon;$food=@($a.previews|Where-Object form -eq 86427)
  if($food.Count){
   $dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1];$len=[Math]::Sqrt($dx*$dx+$dy*$dy)
   $script:foodBasis=@(($dx/$len),($dy/$len))
   Touch-Slot $food[0].slot 1|Out-Null;Start-Sleep -Milliseconds 700
   Assert-Equal (Get-Addon).nativeGrabHand 1 'Selected food uses native right hand';return
  }
  Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300
 }
 throw 'Food not found'
}
function Mouth-Food {
 for($i=0;$i -lt 5;$i++){
  $a=Get-Addon;if(!$a.nativeGrabRef){return}
  $s=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $ex=$a.mouthPoint[0]-$a.heldCenter[0];$ey=$a.mouthPoint[1]-$a.heldCenter[1];$ez=$a.mouthPoint[2]-$a.heldCenter[2]
  $rx=$script:foodBasis[0];$ry=$script:foodBasis[1]
  $px=$s.handR.pos[0]+($rx*$ex+$ry*$ey)/70;$py=$s.handR.pos[1]+$ez/70;$pz=$s.handR.pos[2]-(-$ry*$ex+$rx*$ey)/70
  Send-XR @(("hand r to grip {0:F5} {1:F5} {2:F5} 0 0 0 350" -f $px,$py,$pz))
  Start-Sleep -Milliseconds 500
  if((Get-Addon).mouthDistance -ge 0 -and (Get-Addon).mouthDistance -lt 8.4){break}
 }
 Start-Sleep -Milliseconds 950
}
$initial=Get-Native 'player.GetItemCount 0001519B'
foreach($key in 'x','a'){
 Pick-Food
 Send-XR @("btn $key press 180");Start-Sleep -Milliseconds 650
 Assert-Equal (Get-Addon).nativeGrabRef 0 "$key safely returns sticky food"
 Assert-Equal (Get-Native 'player.GetItemCount 0001519B') $initial "$key returns one real item without duplication"
}
Pick-Food
Assert-Equal (Get-Addon).heldFood $true 'Iguana is recognized as food'
$health=Get-Native 'player.GetAV Health'
Mouth-Food
Save-Eyes 'sticky-food-after-mouth'
Assert-Equal (Get-Addon).nativeGrabRef 0 'Mouth gesture clears consumed native food'
Assert-Equal (Get-Native 'player.GetItemCount 0001519B') ($initial-1) 'Exactly one food consumed'
Start-Sleep -Seconds 2
Assert-Equal ((Get-Native 'player.GetAV Health') -gt $health) $true 'Native food effect heals normally'
Pick-Food
Send-XR @('grip r 1');Start-Sleep -Milliseconds 300
Assert-Equal (Get-Addon).aidGripLatch $false 'Squeeze exits sticky mode'
$nativeHeld=(Get-Addon).nativeGrabRef
Mouth-Food
Assert-Equal (Get-Addon).nativeGrabRef $nativeHeld 'Normal holding does not auto-eat at mouth'
Assert-Equal (Get-Native 'player.GetItemCount 0001519B') ($initial-2) 'Native-mode food remains the held instance'
Send-XR @('input clear')
