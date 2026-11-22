. "$PSScriptRoot\pool-equipment-helpers.ps1"
Load-PoolEquipment

$base = Get-Native 'GetFormFromMod "RobCo Eyebot.esp" "000800"'
Assert-Equal ($base -gt 0) $true 'Standalone Eyebot activator loaded'
$key = '*_RobCoEyebotQA'
Send-Game ('player.AuxVarSetRef "{0}" (player.PlaceAtMe (GetFormFromMod "RobCo Eyebot.esp" "000800") 1 0 0)' -f $key) | Out-Null
$reference = '(player.AuxVarGetRef "{0}")' -f $key
$robot = Get-Native "player.AuxVarGetRef `"$key`""
Assert-Equal ($robot -ne 0) $true 'Native Eyebot reference created'

$x = 345
$y = -169
$z = 3515
$sy = -169
$culture = [System.Globalization.CultureInfo]::InvariantCulture
function Number([double]$value) { $value.ToString('0.###', $culture) }
Send-Game @(
    'SunnyRef.SetPos X 168',
    'SunnyRef.SetPos Y -169',
    "Call ({ref robotRef} => robotRef.SetPos X $(Number $x)) $reference",
    "Call ({ref robotRef} => robotRef.SetPos Y $(Number $y)) $reference",
    "Call ({ref robotRef} => robotRef.SetPos Z $(Number $z)) $reference",
    "Call ({ref robotRef} => robotRef.SetAngle Z 90) $reference"
) | Out-Null
Start-Sleep -Milliseconds 1100

function Eyebot([string]$action) {
    $command = 'Call ({{ref robotRef}} => DispatchEvent "RobCo:Eyebot{0}" (ar_Map "device"::robotRef)) {1}' -f $action,$reference
    Send-Game $command | Out-Null
}

$trace = Join-Path $script:Session 'eyebot-native-capture.json'
if (Test-Path -LiteralPath $trace) { Remove-Item -LiteralPath $trace }
$still = Join-Path $script:Session 'eyebot-native-snapshot.json'
if (Test-Path -LiteralPath $still) { Remove-Item -LiteralPath $still }
Eyebot Place
Eyebot Stay
Eyebot Snap
Start-Sleep -Milliseconds 550
$snapshot = if (Test-Path -LiteralPath $still) { Get-Content -LiteralPath $still -Raw | ConvertFrom-Json }
Write-Host "Snapshot $($snapshot | ConvertTo-Json -Compress)"
Eyebot Record
Start-Sleep -Seconds 1
# Shift Sunny's native actor root across the parked lens to distinguish motion
# history from the list of stationary references in the first sample.
Send-Game @("SunnyRef.SetPos Y $(Number ($sy + 22))") | Out-Null
Start-Sleep -Seconds 2
Eyebot Recall
Start-Sleep -Milliseconds 300
Assert-Equal (Test-Path -LiteralPath $trace) $true 'Eyebot recorded native scene transforms'
$take = Get-Content -LiteralPath $trace -Raw | ConvertFrom-Json
Write-Host ("Captured {0} frames across {1} us. Sample cost {2} us avg, {3} us max." -f $take.frames,$take.durationUs,$take.avgSampleUs,$take.maxSampleUs)
Assert-Equal ($take.frames -ge 20) $true 'Eyebot captured multiple timestamped native samples'
$sunny = Get-Native 'SunnyRef'
Assert-Equal ($sunny -in $take.entities.id) $true 'Eyebot sees Sunny from its parked lens'
Assert-Equal (($take.entities | Where-Object id -eq $sunny).span -gt .1) $true 'Eyebot retained Sunny position changes on the recording clock'
Assert-Equal $snapshot.rendered $true 'One scheduled offscreen render succeeds'
Assert-Equal $snapshot.savedProof $true 'Eyebot camera produces scene pixels beyond its clear colour'
Assert-Equal ($snapshot.cameraForward[0] -lt -.95) $true 'Snapshot is pointed at the table along the parked lens axis'

$export = Join-Path $script:Session 'eyebot-native-export.mp4'
$exportStatus = Join-Path $script:Session 'eyebot-export-status.json'
foreach ($file in $export,$exportStatus) {
    if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
}
Eyebot Save
$deadline = [DateTime]::UtcNow.AddSeconds(28)
while (!(Test-Path -LiteralPath $exportStatus) -and [DateTime]::UtcNow -lt $deadline) {
    Start-Sleep -Milliseconds 250
}
Assert-Equal (Test-Path -LiteralPath $exportStatus) $true 'Eyebot finished the deferred movie job'
$result = Get-Content -LiteralPath $exportStatus -Raw | ConvertFrom-Json
Write-Host "Export $($result | ConvertTo-Json -Compress)"
Assert-Equal $result.status 0 'Deferred MP4 was encoded and published'
Assert-Equal ($result.frames -ge 20) $true 'Export contains the recorded sequence'
Assert-Equal (Test-Path -LiteralPath $export) $true 'Eyebot wrote the color MP4 to disk'
Assert-Equal $result.restoredFrames $result.frames 'Each deferred draw restored the same live scene transforms'

$remote = Get-Native 'GetFormFromMod "RobCo Eyebot.esp" "000801"'
Assert-Equal ($remote -ne 0) $true 'Eyebot Remote is registered as its own inventory form'
Assert-Equal (Get-Native 'player.GetItemCount (GetFormFromMod "RobCo Eyebot.esp" "000801")') 1 'Eyebot Remote appears once in player inventory'
