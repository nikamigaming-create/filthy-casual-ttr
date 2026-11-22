. "$PSScriptRoot\runtime-helpers.ps1"
# Local integration only: downloaded mods and owned-game fixture are not bundled.
Send-XR @('capture off','pace free 60','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
$index=Get-Native 'GetModIndex "OpenNVWeaponComparison.esp"'
Assert-Equal ($index -ge 0 -and $index -lt 255) $true 'Test armory plugin loaded'
Assert-Equal (Get-Native 'player.AuxVarGetFlt "*bLingeringTracers"') 1 'Tracer initialization ran'
Assert-Equal (Get-Native 'player.AuxVarGetFlt "*bTracerInterpolation"') 0 'Tracer interpolation remains disabled'
Send-XR @('input clear');Park-Hands
Send-Game @('coc GSDocMitchellHouse') | Out-Null
Start-Sleep -Seconds 4
foreach($i in 0,1,2,3){
 $hex=('{0:X}' -f (0x900+$i))
 $lookup='GetFormFromMod "OpenNVWeaponComparison.esp" "'+$hex+'"'
 $ref=Get-Native $lookup
 Assert-Equal ($ref -ne 0) $true "Armory reference $hex resolves in loaded cell"
 $count=Get-Native ('Call ({ref box} => box.GetNumItems) ('+$lookup+')')
 Assert-Equal $count (@(302,194,145,112)[$i]) "Armory container $ref contains expected distinct items"
}
Send-Game @('player.SetPos X 1150','player.SetPos Y 1510','player.SetAngle Z 0') | Out-Null
Send-XR @('head to 0 1.1 0 0 -25 0 400')
Start-Sleep -Milliseconds 600
Save-Eyes 'local-armory-installed'
Write-Output 'PASS: local armory and tracer initialization; physical tracer appearance still needs play check'
