. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture')|Out-Null
Start-Sleep -Seconds 6
for($notice=0;$notice -lt 3 -and (Get-Addon).nativeTopMenu -eq 1001;$notice++){
 Send-XR @('btn a press 180');Start-Sleep -Milliseconds 600
}
Park-Hands
foreach($s in @(@('bDrawShadows',1),@('iShadowMapResolution',1024),@('iShadowFilter',2),@('iActorShadowCountInt',2),@('iActorShadowCountExt',2))){
 $value=Get-Native ('GetNumericINISetting "'+$s[0]+':Display"')
 Assert-Equal $value $s[1] ('Effective '+$s[0])
}
Send-Game @('player.MoveTo 00104C79 110 0 0','player.SetAngle z 270')|Out-Null
Start-Sleep -Seconds 3
Save-Eyes 'nvtup-shadows-interior'
