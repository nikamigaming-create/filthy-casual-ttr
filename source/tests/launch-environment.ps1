param([string]$Destination)
$safeNames=@('PATH','PATHEXT','SystemRoot','SystemDrive','ComSpec','ProgramFiles','ProgramFiles(x86)','ProgramW6432','TEMP','TMP','USERPROFILE','LOCALAPPDATA','APPDATA','PROCESSOR_ARCHITECTURE','PROCESSOR_ARCHITEW6432','SteamAppId','SteamGameId','SteamPath','__COMPAT_LAYER','XR_RUNTIME_JSON')
$sha=[Security.Cryptography.SHA256]::Create()
$values=[ordered]@{}
foreach($entry in Get-ChildItem Env: | Sort-Object Name){
 $digest=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($entry.Value)))).Replace('-','')
 $values[$entry.Name]=@{hash=$digest;value=$(if($safeNames -contains $entry.Name){$entry.Value}else{$null})}
}
$sha.Dispose()
$steam=Get-ItemProperty 'HKCU:\Software\Valve\Steam\ActiveProcess' -ErrorAction SilentlyContinue | Select-Object pid,SteamClientDll,SteamClientDll64,Universe
$steamPath=Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue | Select-Object SteamPath,SteamExe
@{bits=[IntPtr]::Size*8;cwd=(Get-Location).Path;variables=$values;steam=$steam;steamPath=$steamPath} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $Destination
