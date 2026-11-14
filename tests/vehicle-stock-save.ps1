. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','input clear','head pose 0 1.1 0 0 -10 0','btn a press 180')
Send-Game @('load FC_Vehicles_Ready')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 4 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 450}
Park-Hands
$forms=Get-Content "$script:Repo\work\vehicle-controls\bottle-forms.json" -Raw|ConvertFrom-Json
$models=@('clutter\liquorbottles\whiskeybottle01.nif','clutter\liquorbottles\whiskeybottle02.nif','clutter\junk\sodabottleclosed01.nif','clutter\junk\nv\nvnukacolaquartz.nif','clutter\junk\nv\nvnukacolavictory.nif','clutter\questitems\ms05quantumnukacola.nif','clutter\junk\ssbottle01.nif')
$stock=@($forms|Where-Object {$_.type -eq 'ALCH' -and $_.MODL.ToLowerInvariant() -in $models}|ForEach-Object {[pscustomobject]@{id=$_.id;name=$_.FULL;minimum=1}})
$stock+=@([pscustomobject]@{id='00015169';name='Stimpak';minimum=5},[pscustomobject]@{id='0000434F';name='10mm pistol';minimum=1},[pscustomobject]@{id='0008F21E';name='Assault carbine';minimum=1},[pscustomobject]@{id='00004241';name='10mm ammunition';minimum=120},[pscustomobject]@{id='0006B53D';name='5mm ammunition';minimum=240})
foreach($item in $stock){
 $n=Get-Native ('player.GetItemCount '+$item.id)
 if($n -lt $item.minimum){Send-Game @('player.AddItem '+$item.id+' '+($item.minimum-$n))|Out-Null}
 Assert-Equal ((Get-Native ('player.GetItemCount '+$item.id)) -ge $item.minimum) $true ('Stocked '+$item.name)
}
Send-Game @('player.SetWeaponOut 0','save FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 2
Send-Game @('load FC_Vehicles_Stocked')|Out-Null;Start-Sleep -Seconds 6
for($i=0;$i -lt 4 -and (Get-Addon).menu;$i++){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 450}
Assert-Equal (Get-Native 'player.GetDead') 0 'Stocked test character alive after reload'
foreach($item in $stock){Assert-Equal ((Get-Native ('player.GetItemCount '+$item.id)) -ge $item.minimum) $true ('Saved inventory retains '+$item.name)}
$stock|ConvertTo-Json -Depth 5|Set-Content "$script:Repo\evidence\vehicle-test-stock.json"
Save-Eyes 'vehicle-stocked-ready'
Write-Output 'PASS: FC_Vehicles_Stocked saved and reloaded; existing parked vehicles preserved'
