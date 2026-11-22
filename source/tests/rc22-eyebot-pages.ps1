. "$PSScriptRoot/equipment-device-input.ps1"

$arm = Join-Path $script:Session 'eyebot-arm-state.json'
function Select-Eyebot([bool]$worn) {
    $page = Page
    Assert-Equal $page.title 'GENERAL SETTINGS' 'FILTHY opens on General'
    Assert-Equal ([bool]$page.onArm) $worn 'General is on the expected device'
    Assert-Equal ([bool]($page.availablePages -band 4)) $true 'The installed recorder supplies its page'
    if ($worn) {
        if (Test-Path -LiteralPath $arm) { Remove-Item -LiteralPath $arm }
        Grip r
        $view = Get-Content -LiteralPath $arm -Raw | ConvertFrom-Json
        Assert-Equal $view.pid $gameId 'The current process opened FILTHY > Eyebot'
    } else {
        Button x
        Save-Eyes 'rc22-eyebot-page' | Out-Host
        $columns = 'pid','time','owner','page','menu','open','onArm','input','ready'
        $focus = Import-Csv -LiteralPath (Join-Path $script:Session 'filthy-focus.csv') -Header $columns |
            Where-Object pid -eq $gameId | Select-Object -Last 1
        Assert-Equal "$($focus.owner)/$($focus.page)/$($focus.menu)/$($focus.open)/$($focus.onArm)" '2/2/1005/1/0' 'Full Pip-Boy owns the open Eyebot page'
    }
}

Send-XR @('input clear','profile touch','head valid on','btn a press 170')
Send-Game 'load SC_QA_fixture' | Out-Null
Start-Sleep -Seconds 7
Send-Game @('CloseAllMenus','player.SetWeaponOut 0') | Out-Null
Send-XR @('head to 0 1.11 0 8 -29 0 560','hand l to grip -.07 .97 -.45 -12 47 152 650',
          'hand r to grip -.045 .96 -.28 0 -15 0 650')
Start-Sleep -Milliseconds 950
Send-XR 'trigger r .99'
Start-Sleep -Milliseconds 300
for ($i=0; $i -lt 6; $i++) { Grip r; if ((Page).onArm -and (Page).title -eq 'GENERAL SETTINGS') { break } }
Send-XR 'trigger r 0'
Start-Sleep -Milliseconds 350
Select-Eyebot $true
Button a
Start-Sleep -Milliseconds 850
Assert-Equal (Get-Content $arm -Raw | ConvertFrom-Json).state 1 'The wrist page deploys the recorder'
Button a
Assert-Equal (Get-Content $arm -Raw | ConvertFrom-Json).state 2 'The wrist page parks the recorder'
Grip l
Send-XR 'trigger l .96'
Start-Sleep -Milliseconds 190
Send-XR 'trigger l 0'
Start-Sleep -Milliseconds 850
Assert-Equal (Get-Native 'GetActiveMenuMode') 1002 'Full Pip-Boy opens to Items'
Send-XR 'hand r to grip .22 .80 -.28 0 -50 0 720'
Start-Sleep -Milliseconds 950
for ($i=0; $i -lt 5 -and (Get-Native 'GetActiveMenuMode') -ne 1005; $i++) { Category }
Select-Eyebot $false
