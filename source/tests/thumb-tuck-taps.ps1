. "$PSScriptRoot\natural-thumb-final.ps1"
# Both mirrored hands and eyes: dorsal/palm views, rest/raised distinction,
# followed by real sensor combinations in the isolated native game.
Send-XR @('input clear','head to 0 1.1 0 0 -25 0 450','hand l to grip -0.16 1.00 -0.34 0 55 0 450','hand r to grip 0.16 1.00 -0.34 0 55 0 450','grip l 1','grip r 1','thumbrest l on','thumbrest r on','touch l index on','touch r index on')
Start-Sleep -Milliseconds 700
Save-Eyes 'thumb-tuck-dorsal'
Send-XR @('hand l to grip -0.16 1.00 -0.34 0 55 -70 650','hand r to grip 0.16 1.00 -0.34 0 55 70 650')
Start-Sleep -Milliseconds 720
Save-Eyes 'thumb-tuck-rotating'
Send-XR @('hand l to grip -0.16 1.00 -0.34 0 55 -140 650','hand r to grip 0.16 1.00 -0.34 0 55 140 650')
Start-Sleep -Milliseconds 720
Save-Eyes 'thumb-tuck-palm'
Send-XR @('input clear');Park-Hands
foreach($h in @('l','r')){
 $other=if($h -eq 'l'){'r'}else{'l'}
 Send-XR @("touch $other stick on");Start-Sleep -Milliseconds 180
 Send-XR @("thumbrest $h on");Start-Sleep -Milliseconds 1200
 Assert-Equal (Get-Addon).openHand -1 'Rest hold no longer opens a wheel'
 Send-XR @("thumbrest $h off");Start-Sleep -Milliseconds 100
 Send-XR @("thumbrest $h on");Start-Sleep -Milliseconds 90
 Send-XR @("thumbrest $h off");Start-Sleep -Milliseconds 110
 Send-XR @("thumbrest $h on");Start-Sleep -Milliseconds 100
 Assert-Equal (Get-Addon).openHand $(if($h -eq 'l'){0}else{1}) "Double-tap $h with opposite stick selects the correct wheel"
 Send-XR @('hand l to grip -0.20 0.95 -0.34 0 55 0 450','hand r to grip 0.20 0.95 -0.34 0 55 0 450','thumbrest l on','thumbrest r on','touch l index on','touch r index on','grip l 0','grip r 0')
 Start-Sleep -Milliseconds 600;$open=Get-Addon
 Save-Eyes "wheel-$h-open-hand"
 Send-XR @('grip l 1','grip r 1');Start-Sleep -Milliseconds 700;$closed=Get-Addon
 foreach($hand in 0,1){
  Assert-Equal ($closed.fingerGestures[$hand].applied -gt $open.fingerGestures[$hand].applied) $true "Wheel $h hand $hand gesture pass remains active"
  foreach($finger in 2,3,4){
   $sum=0;foreach($axis in 0,1,2){$d=$closed.fingerGestures[$hand].tips[$finger][$axis]-$open.fingerGestures[$hand].tips[$finger][$axis];$sum+=$d*$d}
   Assert-Equal ($sum -gt .25) $true "Wheel $h hand $hand finger $finger bends despite blocked gameplay grip"
  }
 }
 Assert-Equal (Get-Native 'player.IsWeaponOut') 0 'Wheel grip animation does not draw a weapon'
 Assert-Equal $closed.nativeGrabRef 0 'Wheel grip animation does not grab an item'
 Save-Eyes "wheel-$h-closed-hand"
 Send-XR @('btn b press 160');Start-Sleep -Milliseconds 250
 Send-XR @('input clear');Start-Sleep -Milliseconds 180
}
Write-Output 'PASS: thumb closure views and left/right double-tap selectors'
