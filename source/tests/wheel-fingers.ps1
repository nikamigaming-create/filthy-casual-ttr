. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600}
Send-Game @('player.SetWeaponOut 0') | Out-Null
foreach($h in @('l','r')){
 Send-XR @('input clear','head to 0 1.1 0 0 0 0 400');Park-Hands
 $other=if($h -eq 'l'){'r'}else{'l'}
 $expected=if($h -eq 'l'){0}else{1}
 Send-XR @("thumbrest $other on");Start-Sleep -Milliseconds 180
 Send-XR @("thumbrest $h on");Start-Sleep -Milliseconds 90
 Send-XR @("thumbrest $h off");Start-Sleep -Milliseconds 110
 Send-XR @("thumbrest $h on");Start-Sleep -Milliseconds 150
 Assert-Equal (Get-Addon).openHand $expected "Wheel $h opens through double tap"
 # Keep both hands close to the torso, behind the floating previews.
 Send-XR @('head to 0 1.1 0 0 -45 0 450','hand l to grip -0.19 0.88 -0.20 0 55 0 450','hand r to grip 0.19 0.88 -0.20 0 55 0 450','thumbrest l on','thumbrest r on','touch l index on','touch r index on','grip l 0','grip r 0')
 Start-Sleep -Milliseconds 650;$open=Get-Addon
 Assert-Equal $open.openHand $expected "Wheel $h remains open with free hands"
 Assert-Equal $open.nativeGrabRef 0 "Wheel $h starts without held item"
 Save-Eyes "wheel-$h-open-hand"
 Send-XR @('grip l 1','grip r 1');Start-Sleep -Milliseconds 700;$closed=Get-Addon
 Assert-Equal $closed.openHand $expected "Wheel $h stays open through squeeze"
 foreach($hand in 0,1){
  Assert-Equal ($closed.fingerGestures[$hand].applied -gt $open.fingerGestures[$hand].applied) $true "Wheel $h hand $hand gesture pass remains active"
  foreach($finger in 0,1,2,3,4){
   $sum=0;foreach($axis in 0,1,2){$d=$closed.fingerGestures[$hand].tips[$finger][$axis]-$open.fingerGestures[$hand].tips[$finger][$axis];$sum+=$d*$d}
   Assert-Equal ($sum -gt .25) $true "Wheel $h hand $hand finger $finger bends"
  }
 }
 Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Wheel squeeze does not draw a weapon'
 Assert-Equal $closed.nativeGrabRef 0 'Wheel squeeze does not grab an item'
 Save-Eyes "wheel-$h-closed-hand"
 Send-XR @('input clear','btn b press 160');Start-Sleep -Milliseconds 250
 Assert-Equal (Get-Addon).openHand -1 "Wheel $h closes normally"
}
Write-Output 'PASS: all five fingers on both free hands articulate in both wheels without gameplay grabs'
