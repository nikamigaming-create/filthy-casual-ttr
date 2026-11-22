param([Parameter(Mandatory)][string]$Test, [int]$MaxSeconds=180, [switch]$DesktopLaunch)
$ErrorActionPreference='Stop'
. "$PSScriptRoot\runtime-helpers.ps1"
if(Get-Process FalloutNV -ErrorAction SilentlyContinue){throw 'A game is already running; refusing to start another'}
$action=(Resolve-Path -LiteralPath $Test).Path
if(!$action.StartsWith($PSScriptRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Test must be inside this repository tests directory'}
$parseTokens=$null;$parseErrors=$null
[System.Management.Automation.Language.Parser]::ParseFile($action,[ref]$parseTokens,[ref]$parseErrors)|Out-Null
if($parseErrors){throw ($parseErrors|Out-String)}
$started=[DateTime]::UtcNow
$qaId=0
$moIni='D:\TTWVR\MO2\ModOrganizer.ini'
$originalProfile=[regex]::Match([IO.File]::ReadAllText($moIni),'(?m)^selected_profile=.*$').Value
try {
    $pending=Join-Path $script:Session 'game-command.txt'
    if(Test-Path -LiteralPath $pending){Remove-Item -LiteralPath $pending}
    if($DesktopLaunch){
        # Steam registered in the ordinary desktop session is invisible to the
        # packaged automation registry view. Let the existing desktop shell
        # start this fixed local file; do not restart Steam inside automation.
        Start-Process -FilePath 'explorer.exe' -ArgumentList ('"'+$script:Repo+'\launch-simulator.cmd"') | Out-Null
    }else{
        Start-Process -FilePath 'cmd.exe' -ArgumentList ('/d /c ""'+$script:Repo+'\launch-simulator.cmd""') -WindowStyle Hidden | Out-Null
    }
    $deadline=[DateTime]::UtcNow.AddSeconds(65)
    do {
        Start-Sleep -Milliseconds 250
        if(!$qaId){
            $game=Get-Process FalloutNV -ErrorAction SilentlyContinue | Where-Object {$_.StartTime.ToUniversalTime() -ge $started -and $_.Path -eq 'D:\SteamLibrary\steamapps\common\Fallout New Vegas\FalloutNV.exe'} | Select-Object -First 1
            if($game){
                $qaId=$game.Id
                # Arm at process creation, including the startup/readiness wait.
                Start-Process -FilePath 'powershell.exe' -ArgumentList @('-NoProfile','-File',("`"$PSScriptRoot\watch-qa.ps1`""),'-GameId',$qaId,'-StartTicks',$game.StartTime.ToUniversalTime().Ticks,'-Seconds',$MaxSeconds) -WindowStyle Hidden | Out-Null
            }
        }
        try {
            $state=Get-Content "$script:Session\state.json" -Raw | ConvertFrom-Json
            if($qaId -and $state.pid -eq $qaId -and $state.sessionRunning){break}
        } catch {}
    } while([DateTime]::UtcNow -lt $deadline)
    if(!$qaId -or !$state.sessionRunning -or $state.pid -ne $qaId){throw 'Isolated game did not become ready within 65 seconds'}
    & $action
} finally {
    if(!$qaId){
        $candidate=Get-Process FalloutNV -ErrorAction SilentlyContinue | Where-Object {$_.StartTime.ToUniversalTime() -ge $started -and $_.Path -eq 'D:\SteamLibrary\steamapps\common\Fallout New Vegas\FalloutNV.exe'} | Select-Object -First 1
        if($candidate){$qaId=$candidate.Id}
    }
    if($qaId){
        try {Send-Game @('qqq') | Out-Null} catch {Write-Warning $_}
        $game=Get-Process -Id $qaId -ErrorAction SilentlyContinue
        if($game -and !$game.WaitForExit(3000)){
            if($game.Path -eq 'D:\SteamLibrary\steamapps\common\Fallout New Vegas\FalloutNV.exe' -and $game.StartTime.ToUniversalTime() -ge $started){Stop-Process -Id $qaId -Force}
        }
        Write-Output "QA session closed (PID $qaId)"
    }
    # The CLI-selected QA profile must not become the user's next play profile.
    for($wait=0;$wait -lt 30 -and (Get-Process ModOrganizer -ErrorAction SilentlyContinue);$wait++){Start-Sleep -Milliseconds 100}
    if($originalProfile -and !(Get-Process ModOrganizer -ErrorAction SilentlyContinue)){
        $moText=[IO.File]::ReadAllText($moIni)
        [IO.File]::WriteAllText($moIni,[regex]::Replace($moText,'(?m)^selected_profile=.*$',$originalProfile))
    }
}
