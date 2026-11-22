param([int]$GameId,[long]$StartTicks,[int]$Seconds=180)
$deadline=[DateTime]::UtcNow.AddSeconds([Math]::Min(900,[Math]::Max(10,$Seconds)))
while([DateTime]::UtcNow -lt $deadline){
    $game=Get-Process -Id $GameId -ErrorAction SilentlyContinue
    if(!$game -or $game.StartTime.ToUniversalTime().Ticks -ne $StartTicks){exit}
    Start-Sleep -Milliseconds 500
}
$game=Get-Process -Id $GameId -ErrorAction SilentlyContinue
if($game -and $game.StartTime.ToUniversalTime().Ticks -eq $StartTicks -and $game.Path -eq 'D:\SteamLibrary\steamapps\common\Fallout New Vegas\FalloutNV.exe'){
    Stop-Process -Id $GameId -Force
}
