function Wheel { Get-Content "$script:Session\vehicle-state.json" -Raw | ConvertFrom-Json }
function Move-Rim([double]$Degrees,[switch]$LeftOnly) {
    $w=Wheel
    $theta=$Degrees*[Math]::PI/180
    for ($hand=0; $hand -lt 2; ++$hand) {
        if ($LeftOnly -and $hand -eq 1) { break }
        $sign=if($hand){1}else{-1}
        $side=if($hand){'r'}else{'l'}
        $state=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
        $pose=if($hand){$state.handR.pos}else{$state.handL.pos}
        $target=for($axis=0;$axis -lt 3;++$axis){
            $w.center[$axis]+$sign*([Math]::Cos($theta)*($w.right[$axis]-$w.center[$axis])+[Math]::Sin($theta)*($w.up[$axis]-$w.center[$axis]))
        }
        $point=for($axis=0;$axis -lt 3;++$axis){$pose[$axis]+$target[$axis]-$w.palms[$hand][$axis]}
        Send-XR ('hand {0} to grip {1:F4} {2:F4} {3:F4} {4:F4} 12 0 520' -f $side,$point[0],$point[1],$point[2],$script:wheelYaw)
    }
    Start-Sleep -Milliseconds 760
}
