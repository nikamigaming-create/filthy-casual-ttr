function Touch-Slot([int]$Slot,[int]$Hand=1,[double[]]$Right=@()) {
    $touchHand=if($Hand -eq 1){'r'}else{'l'}; Send-XR @("hand $touchHand grip rot 0 55 0"); Start-Sleep -Milliseconds 250
    $a=Get-Addon
    if ($a.previews.Count -lt 2 -and $Right.Count -ne 2) { throw 'Touch-Slot needs two items or a previously measured right vector' }
    $target=@($a.previews | Where-Object slot -eq $Slot)[0].center
    if($Right.Count -eq 2){$rx=$Right[0];$ry=$Right[1]}else{
        $dx=$a.previews[1].center[0]-$a.previews[0].center[0]
        $dy=$a.previews[1].center[1]-$a.previews[0].center[1]
        $length=[Math]::Sqrt($dx*$dx+$dy*$dy)
        if($length -le 0.001){throw 'Touch-Slot needs a measured right vector for vertically aligned previews'}
        $rx=$dx/$length; $ry=$dy/$length
    }
    $h=if($Hand -eq 1){'r'}else{'l'}
    for($attempt=0;$attempt -lt 6;$attempt++) {
        $touchHand=if($Hand -eq 1){'r'}else{'l'}; Send-XR @("hand $touchHand grip rot 0 55 0"); Start-Sleep -Milliseconds 250
    $a=Get-Addon
        if($a.openHand -lt 0){return $a}
        $state=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
        $pose=if($Hand -eq 1){$state.handR.pos}else{$state.handL.pos}
        $ex=$target[0]-$a.palms[$Hand][0];$ey=$target[1]-$a.palms[$Hand][1];$ez=$target[2]-$a.palms[$Hand][2]
        $px=$pose[0]+($rx*$ex+$ry*$ey)/70
        $py=$pose[1]+$ez/70
        $pz=$pose[2]-(-$ry*$ex+$rx*$ey)/70
        if($attempt -lt 2){$pz+=@(.35,.15)[$attempt]}
        Send-XR @(("hand {0} grip pos {1:F5} {2:F5} {3:F5}" -f $h,$px,$py,$pz))
        Start-Sleep -Milliseconds 650
    }
    return Get-Addon
}
