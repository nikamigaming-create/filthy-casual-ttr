. "$PSScriptRoot\magazine-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -20 0','btn a press 180')
Send-Game @('load SC_PhysicalParts_QA')|Out-Null;Start-Sleep -Seconds 5
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
$a=Get-Addon;$ids=@($a.physicalParts|ForEach-Object ref);$id=('{0:X8}' -f [uint32]$ids[0])
$cell=Get-Native 'player.GetParentCell'
function Part-Cell {Get-Native ('Call ({ref partRef} => partRef.GetParentCell) (StringToRef "'+$id+'")')}
Write-Output "Before playerCell=$cell partCell=$(Part-Cell)"
Send-Game @('coc GSDocMitchellHouse')|Out-Null;Start-Sleep -Seconds 4
Send-Game @('pcb')|Out-Null;Start-Sleep -Seconds 1
Write-Output "Away playerCell=$(Get-Native 'player.GetParentCell') partCell=$(Part-Cell)"
Send-Game @('coc GSGeneralStore')|Out-Null;Start-Sleep -Seconds 6
Write-Output "Return playerCell=$(Get-Native 'player.GetParentCell') partCell=$(Part-Cell)"
$a=Get-Addon;$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-cell-return.json"
Assert-Equal (Get-Native 'player.GetParentCell') $cell 'Native transition returns to the original cell'
Assert-Equal @($a.physicalParts|Where-Object ref -in $ids).Count $ids.Count 'Fitted part survives cell unload and return'
# Hand-pulled installed magazine: native grip -> pull -> release.
Send-Game @('load SC_QA_fixture')|Out-Null;Start-Sleep -Seconds 4
Park-Hands;Send-Game @('sc_weapons')|Out-Null;Start-Sleep -Milliseconds 350
Send-Game @('sc_pick 0000434F')|Out-Null;Start-Sleep -Seconds 1
if(!(Get-Addon).manualReload){Send-Game @('sc_reload')|Out-Null}
Send-XR @('hand r grip pose 0.3 0.95 -0.45 0 55 0','hand l grip pose -0.4 0.8 -0.3 0 30 0');Start-Sleep -Seconds 1
$a=Get-Addon;$before=$a.physicalMags.installedDrops;$t=$a.installedPartTracking
Send-XR @(("hand l grip pos {0:F5} {1:F5} {2:F5}" -f $t[0],($t[1]-$a.heightOffset),$t[2]),'grip l 1');Start-Sleep -Milliseconds 400
Send-XR @('btn a press 180');Start-Sleep -Milliseconds 350
Send-XR @('hand l to grip -0.4 0.8 -0.55 0 30 0 700');Start-Sleep -Milliseconds 900
$a=Get-Addon;$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-hand-pull-before-release.json"
Send-XR @('grip l 0');Start-Sleep -Seconds 2
$a=Get-Addon;$a|ConvertTo-Json -Depth 10|Set-Content "$script:Repo\evidence\magazine-hand-pull-after-release.json"
Assert-Equal ($a.physicalMags.installedDrops -gt $before) $true 'Native hand-pulled installed magazine becomes physical'
