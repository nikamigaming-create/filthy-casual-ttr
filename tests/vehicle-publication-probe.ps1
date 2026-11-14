. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -10 0','btn a press 180')
Send-Game @('load SC_QA_scope_autosave')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 3 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Park-Hands
Send-Game @('tdetect','player.SetWeaponOut 0','set GameHour to 12','player.SetPos X -72500','player.SetPos Y 2000','player.SetAngle Z 0')|Out-Null;Start-Sleep -Seconds 1
$car=Get-Native 'player.PlaceAtMe (GetFormFromMod "UmDrivingVIIIb.esp" "00E6BC") 1 300 0'
Assert-Equal ($car -ne 0) $true 'Native roadster spawned from original car mod'
$id=('{0:X8}' -f $car)
$fn='Call (CompileScript "FCVehicleTest.txt") "'+$id+'" '
Get-Native ($fn+'4')|Write-Output
Get-Native ($fn+'5')|Write-Output
Get-Native ($fn+'3')|Write-Output
Start-Sleep -Seconds 1
$a=Get-Addon
$rx=$a.nativeGripPoints[1][0]-$a.nativeGripPoints[0][0];$ry=$a.nativeGripPoints[1][1]-$a.nativeGripPoints[0][1]
$heading=[Math]::Atan2(-$ry,$rx)*180/[Math]::PI
Get-Native ('Call (CompileScript "FCVehicleAlign.txt") "'+$id+'" '+$heading.ToString('F3',[Globalization.CultureInfo]::InvariantCulture))|Write-Output
Start-Sleep -Milliseconds 500
Get-Native ($fn+'1')|Write-Output;Start-Sleep -Seconds 1
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 3 'Native vehicle driving state entered'

function Wheel-State { Get-Content "$script:Session\vehicle-state.json" -Raw|ConvertFrom-Json }
$w=Wheel-State
$yaw=[Math]::Atan2(-$w.center[0],-$w.center[2])*180/[Math]::PI
Send-XR @("head to 0 1.1 0 $yaw -40 0 700","hand l grip rot $yaw 10 0","hand r grip rot $yaw 10 0",'touch l index on','touch r index on','thumbrest l on','thumbrest r on')
Start-Sleep -Milliseconds 800
function Move-Rim([double]$Degrees,[int]$Duration=600){
 $w=Wheel-State;$theta=$Degrees*[Math]::PI/180
 for($hand=0;$hand -lt 2;$hand++){
  $sign=if($hand -eq 0){-1}else{1};$h=if($hand -eq 0){'l'}else{'r'}
  $t=@();for($j=0;$j -lt 3;$j++){$t+=($w.center[$j]+$sign*([Math]::Cos($theta)*($w.right[$j]-$w.center[$j])+[Math]::Sin($theta)*($w.up[$j]-$w.center[$j])))}
  $state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
  $pose=if($hand -eq 0){$state.handL.pos}else{$state.handR.pos}
  $p=@();for($j=0;$j -lt 3;$j++){$p+=($pose[$j]+$t[$j]-$w.palms[$hand][$j])}
  Send-XR @(("hand {0} to grip {1:F5} {2:F5} {3:F5} {4:F3} 10 0 {5}" -f $h,$p[0],$p[1],$p[2],$yaw,$Duration))
 }
 Start-Sleep -Milliseconds ($Duration+250)
}
Save-Eyes 'vehicle-seated-alignment'
Move-Rim 0
Move-Rim 0 200
Send-XR @('grip l .9','grip r .9');Start-Sleep -Milliseconds 350
for($settle=0;$settle -lt 4;$settle++){Move-Rim 0 350}
$w=Wheel-State;$w|ConvertTo-Json -Depth 6|Write-Output
Assert-Equal ($w.grabbed[0] -eq 1 -and $w.grabbed[1] -eq 1) $true 'Both tracked hands grip the steering rim'
. "$PSScriptRoot\casual-guide-helpers.ps1"
$script:CaptureAddonPath='D:\TTWVR\MO2\mods\TTWVR - Vertibird Development\NVSE\Plugins\SeatedControls.dll'
Start-Take 'roadster-hands-on-wheel' 70
Mark 'Hands on the steering wheel'
Save-Eyes 'vehicle-wheel-gripped'
Start-Sleep -Seconds 1
Mark 'Turn the wheel'
Move-Rim 40 2400
Start-Sleep -Seconds 1
$turn=Wheel-State
Assert-Equal ([Math]::Abs($turn.steer) -gt .25) $true 'Turning the physical wheel produces native steering'
Save-Eyes 'vehicle-wheel-turned'
Move-Rim 0 2200
Start-Sleep -Seconds 1
Mark 'Drive with Quest controls'
$start=(Wheel-State).position
Send-XR @('trigger r .5')
$motion=@();for($i=0;$i -lt 20;$i++){Start-Sleep -Milliseconds 200;$motion+=Wheel-State}
$motion|ConvertTo-Json -Depth 8|Set-Content "$script:Repo\work\vehicles\rim-motion.json"

$drive=Wheel-State
Assert-Equal ($drive.gas -gt .4) $true 'Quest trigger reaches native throttle'
Assert-Equal ([Math]::Abs($drive.position[0]-$start[0])+[Math]::Abs($drive.position[1]-$start[1]) -gt 10) $true 'Native car physics moves the vehicle'
Write-Output ('Lost contact samples '+@($motion|Where-Object {$_.grabbed[0] -ne 1 -or $_.grabbed[1] -ne 1}).Count)
Save-Eyes 'vehicle-driving-contact'
Mark 'Brake and stop'
Send-XR @('trigger r 0','trigger l 1');Start-Sleep -Seconds 1
Start-Sleep -Seconds 1
Mark 'Leave the vehicle'
Send-XR @('grip l 0','grip r 0','trigger l 0','btn b press 200');Start-Sleep -Seconds 1
Assert-Equal ((Get-Content "$script:Session\vehicle-probe.json" -Raw|ConvertFrom-Json).driving) 0 'Quest B exits the native vehicle'
Assert-Equal (Get-Native 'player.GetDead') 0 'Driver survives the drive'

Start-Sleep -Seconds 1
Finish-Take
Stop-Take
