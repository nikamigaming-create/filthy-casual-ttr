$ErrorActionPreference = 'Stop'
$script:Repo = Split-Path $PSScriptRoot -Parent
$script:Session = Join-Path $script:Repo 'sim-session'
$script:GameLog = 'D:\SteamLibrary\steamapps\common\Fallout New Vegas\SeatedControls.log'
function Send-XR([string[]]$Lines) {
    & "$script:Repo\xrsim\xrsim-cmd.ps1" -Dir $script:Session -Lines $Lines -Quiet | Out-Null
}
function Send-Game([string[]]$Lines) {
    $state = Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
    if (!$state.sessionRunning -or !(Get-Process -Id $state.pid -ErrorAction SilentlyContinue)) { throw 'No live isolated simulator' }
    $path = Join-Path $script:Session 'game-command.txt'
    if (Test-Path -LiteralPath $path) { throw 'A native game command is already pending' }
    $before = @(Get-Content -LiteralPath $script:GameLog).Count
    [IO.File]::WriteAllLines($path, $Lines)
    $deadline = [DateTime]::UtcNow.AddSeconds(8)
    while (Test-Path -LiteralPath $path) {
        if ([DateTime]::UtcNow -gt $deadline) { throw 'Native mailbox timed out' }
        Start-Sleep -Milliseconds 50
    }
    Start-Sleep -Milliseconds 80
    return @(Get-Content -LiteralPath $script:GameLog | Select-Object -Skip $before)
}
function Get-Addon {
    Send-Game @('sc_state') | Out-Null
    return Get-Content "$script:Session\addon-state.json" -Raw | ConvertFrom-Json
}
function Get-Native([string]$Expression) {
    $lines = Send-Game @("expr $Expression")
    $result = $lines | Where-Object { $_ -like '*TEST EXPR*' } | Select-Object -Last 1
    if ($result -notmatch 'ok=1 type=(\d+) number=([^ ]+) form=([0-9A-F]+)') { throw "Native expression failed: $Expression : $result" }
    if ($Matches[1] -eq '2') { return [Convert]::ToInt32($Matches[3],16) }
    if ($Matches[1] -ne '1') { throw "Native expression did not return a number or form: $Expression : $result" }
    return [double]::Parse($Matches[2],[Globalization.CultureInfo]::InvariantCulture)
}
function Save-Eyes([string]$Name) {
    & "$script:Repo\xrsim\xrsim-shot.ps1" -Dir $script:Session -Out "$script:Repo\evidence\$Name" -Quiet |
        Select-Object Frame,SessionState,Layers,NonBlackPctL,NonBlackPctR
}
function Assert-Equal($Actual,$Expected,[string]$Claim) {
    $record=[ordered]@{time=[DateTime]::UtcNow.ToString('o');claim=$Claim;actual=$Actual;expected=$Expected;pass=($Actual -eq $Expected)}
    $record | ConvertTo-Json -Compress -Depth 5 | Add-Content -LiteralPath "$script:Repo\evidence\assertions.jsonl"
    if ($Actual -ne $Expected) { throw "$Claim : expected $Expected, got $Actual" }
    Write-Output "PASS: $Claim ($Actual)"
}
function Park-Hands {
    Send-XR @('hand l grip pose -0.35 0.60 -0.10 0 0 0','hand r grip pose 0.35 0.60 -0.10 0 0 0','trigger l 0','trigger r 0','grip l 0','grip r 0')
    Start-Sleep -Milliseconds 180
}
function Touch-Slot([int]$Slot,[int]$Hand=1,[double[]]$Right=@()) {
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
    for($attempt=0;$attempt -lt 4;$attempt++) {
        $a=Get-Addon
        if($a.openHand -lt 0){return $a}
        $state=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
        $pose=if($Hand -eq 1){$state.handR.pos}else{$state.handL.pos}
        $ex=$target[0]-$a.palms[$Hand][0];$ey=$target[1]-$a.palms[$Hand][1];$ez=$target[2]-$a.palms[$Hand][2]
        $px=$pose[0]+($rx*$ex+$ry*$ey)/70
        $py=$pose[1]+$ez/70
        $pz=$pose[2]-(-$ry*$ex+$rx*$ey)/70
        Send-XR @(("hand {0} grip pos {1:F5} {2:F5} {3:F5}" -f $h,$px,$py,$pz))
        Start-Sleep -Milliseconds 220
    }
    return Get-Addon
}
