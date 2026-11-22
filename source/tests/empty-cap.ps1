. "$PSScriptRoot\runtime-helpers.ps1"
Send-XR @('capture off','pace free 60','head height 1.1','btn a press 180')
Send-Game @('load SC_QA_fixture') | Out-Null
Start-Sleep -Seconds 4
if((Get-Addon).menu){Send-XR @('btn a press 180');Start-Sleep -Milliseconds 300}
Park-Hands
Send-XR @('click r press 180')
Start-Sleep -Milliseconds 350
$a=Get-Addon
Assert-Equal $a.previews.Count 7 'Populated weapon page retains six choices and center cap'
$dx=$a.previews[1].center[0]-$a.previews[0].center[0];$dy=$a.previews[1].center[1]-$a.previews[0].center[1]
$length=[Math]::Sqrt($dx*$dx+$dy*$dy);$right=@(($dx/$length),($dy/$length))
Touch-Slot 6 1 | Out-Null
Assert-Equal (Get-Addon).openHand -1 'Populated selector cap closes normally'
Send-Game @('player.RemoveAllItems') | Out-Null
foreach($hand in @('l','r')) {
    Park-Hands
    Send-XR @("click $hand press 180")
    Start-Sleep -Milliseconds 350
    Assert-Equal (Get-Addon).previews.Count 1 "Empty $hand inventory still has center cap"
    Assert-Equal (Get-Addon).popupVisible $false "Empty $hand selector is quiet"
    Send-XR @('btn a press 180')
    Start-Sleep -Milliseconds 250
    Assert-Equal (Get-Addon).previews.Count 1 "Paging empty $hand selector is safe"
    if($hand -eq 'l'){Save-Eyes 'cap-empty-inventory'}
    Touch-Slot 6 1 $right | Out-Null
    Assert-Equal (Get-Addon).openHand -1 "Empty $hand selector cap closes cleanly"
}
