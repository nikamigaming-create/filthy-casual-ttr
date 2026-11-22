. "$PSScriptRoot\runtime-helpers.ps1"
$state=Get-Content "$script:Session\state.json" -Raw|ConvertFrom-Json
$out="$script:Repo\evidence\food-x-crash"
New-Item -ItemType Directory -Path $out -Force|Out-Null
$watch=Start-Process -FilePath 'C:\Users\nbrys\AppData\Local\Programs\Python\Python311\python.exe' -ArgumentList @('C:\Users\nbrys\Documents\Codex\2026-11-07\ho\watch_food_crash.py',$state.pid,$out) -WindowStyle Hidden -PassThru
Start-Sleep -Seconds 1
Send-XR @('capture off','pace free 60','head height 1.1','input clear','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 400}
Send-Game @('player.AddItem 0001519B 3')|Out-Null
Park-Hands
Send-Game @('sc_aid')|Out-Null
Start-Sleep -Milliseconds 350
for($page=0;$page -lt 5;$page++){
 $a=Get-Addon;$food=@($a.previews|Where-Object form -eq 86427)
 if($food.Count){Touch-Slot $food[0].slot 1|Out-Null;break}
 Send-XR @('btn a press 180');Start-Sleep -Milliseconds 350
}
Start-Sleep -Milliseconds 700
$before=Get-Addon
Assert-Equal $before.nativeGrabHand 1 'Iguana on a Stick held in right hand'
Assert-Equal $before.aidGripLatch $true 'Sticky hold active'
Send-XR @('btn x press 180');Start-Sleep -Seconds 2
$after=Get-Addon
$after|ConvertTo-Json -Depth 10|Set-Content "$out\after.json"
Write-Output "X completed: held=$($after.nativeGrabRef) count=$(Get-Native 'player.GetItemCount 0001519B')"
# Reproduce X/Take when its activation ray actually hits the held reference.
$ref=('{0:X8}' -f [uint32]$after.nativeGrabRef)
Send-Game @('expr Call ({ref item} => item.Activate player) (StringToRef "'+$ref+'")')|Out-Null
Start-Sleep -Seconds 2
Write-Output "Activate completed: held=$((Get-Addon).nativeGrabRef)"
