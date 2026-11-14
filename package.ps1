param([ValidatePattern('^[A-Za-z0-9._-]+\.zip$')][string]$OutputName = 'Filthy-Casual-TTR-1.8.0-beta.1.zip', [string]$ReleaseCommit='')
$ErrorActionPreference = 'Stop'
$repo = $PSScriptRoot
$out = Join-Path $repo 'outputs'

# Direct allowlisted ZIP creation: no reused staging tree or enclosing mod folder.
# Destination paths are relative to MO2's data root and use ZIP-standard slashes.
$files = [ordered]@{
    'NVSE/Plugins/SeatedControls.dll' = 'build/SeatedControls.dll'
    'NVSE/Plugins/SeatedControls.ini' = 'SeatedControls.ini'
    'NVSE/Plugins/SeatedControls/SeatedInput.dll' = 'build/SeatedInput.dll'
    'NVSE/Plugins/SeatedControls/SeatedInput.json' = 'SeatedInput.json'
    'Filthy Casual Reload Parts.esp' = 'build/reload-parts/mod/Filthy Casual Reload Parts.esp'
    'meshes/FilthyCasual/reload-carrier.nif' = 'build/reload-parts/mod/meshes/FilthyCasual/reload-carrier.nif'
    'textures/FilthyCasual/reload-carrier.dds' = 'build/reload-parts/mod/textures/FilthyCasual/reload-carrier.dds'
    'Filthy Casual Glass.esp' = 'build/glass-assets/Filthy Casual Glass.esp'
    'textures/FilthyCasual/Glass/glass-impact-v1.dds' = 'build/glass-assets/textures/FilthyCasual/Glass/glass-impact-v1.dds'
    'textures/FilthyCasual/Glass/glass-flat_n.dds' = 'build/glass-assets/textures/FilthyCasual/Glass/glass-flat_n.dds'
    'docs/bottle-impacts.md' = 'docs/bottle-impacts.md'
    'docs/PHYSICAL_PARTS.md' = 'docs/PHYSICAL_PARTS.md'
    'README.md' = 'README.md'
    'docs/INSTALL.md' = 'docs/INSTALL.md'
    'docs/assets/terminal-banner.svg' = 'docs/assets/terminal-banner.svg'
    'meta.ini' = 'meta.ini'
    'THIRD_PARTY.md' = 'THIRD_PARTY.md'
    'third_party/openxr/LICENSES/Apache-2.0.txt' = 'third_party/openxr/LICENSES/Apache-2.0.txt'
    'third_party/openxr/LICENSES/MIT.txt' = 'third_party/openxr/LICENSES/MIT.txt'
}
# Explicitly licensed controller models plus our generated terminal artwork.
foreach($asset in 'left.mesh','right.mesh','left.png','right.png','robco-terminal.png','robco-compact.png','LICENSE.txt','LICENSE-MGS5VR.txt','provenance.json','filthy-casual-provenance.json'){
 $files['NVSE/Plugins/SeatedControls/Controls/'+$asset]='assets/controls/'+$asset
}
foreach($name in 'controls','weapons','aid','height','driving','flight'){$files['docs/assets/robco-'+$name+'.png']='docs/assets/robco-'+$name+'.png'}
$files['docs/VALIDATION.md']='docs/VALIDATION.md'
$files['docs/CONTROLS-EDITOR.md']='docs/CONTROLS-EDITOR.md'
$files['docs/CONTROLS-PROVIDERS.md']='docs/CONTROLS-PROVIDERS.md'
$files['tools/controls-editor.html']='tools/controls-editor.html'
$files['Configure Controls.cmd']='Configure Controls.cmd'
# Known controller assets and typed starter profiles; original/MIT only.
foreach($controller in 'oculus-touch-v3','meta-quest-touch-plus','meta-quest-touch-pro','valve-index','htc-vive','microsoft-mixed-reality','pico-4','generic-trigger','steam-frame'){
 foreach($file in 'left.mesh','right.mesh','left.png','right.png'){
  $files['NVSE/Plugins/SeatedControls/Controls/'+$controller+'/'+$file]='assets/controls/'+$controller+'/'+$file
 }
}
foreach($file in 'controller-profiles.json','controller-models-provenance.json','LICENSE-webxr-controllers.md'){$files['NVSE/Plugins/SeatedControls/Controls/'+$file]='assets/controls/'+$file}
foreach($id in 'touch','plus','pro','index','vive','wmr','pico','frame','simple'){$files['profiles/controllers/'+$id+'.ini']='profiles/controllers/'+$id+'.ini'}
$files['docs/CONTROLLERS.md']='docs/CONTROLLERS.md'
$files['docs/CURRENT-BETA.md']='docs/CURRENT-BETA.md'
$files['docs/CREATURE-RIDES.md']='docs/CREATURE-RIDES.md'
foreach($asset in 'controllers-overview.jpg','controllers-montage-mobile.mp4','filthy-casual-field-manual.png'){$files['docs/assets/'+$asset]='docs/assets/'+$asset}
# Original tutorial assets only: derive the explicit card names from the model,
# never recurse over the game installation or a development staging directory.
$header=[IO.File]::ReadAllText((Join-Path $repo 'reload_tutorial.hpp'))
$section=[regex]::Match($header,'cardNames\[\]=\{([^}]+)\}').Groups[1].Value
$cards=@([regex]::Matches($section,'"([a-z-]+)"')|ForEach-Object {$_.Groups[1].Value})
if($cards.Count -lt 10 -or $cards.Count -gt 64 -or ($cards|Sort-Object -Unique).Count -ne $cards.Count){throw 'Invalid tutorial card allowlist'}
$files['meshes/FilthyCasual/tutorial.nif']='build/tutorial-assets/meshes/FilthyCasual/tutorial.nif'
foreach($card in @($cards)+@('line','flat_n')){
 $name='textures/FilthyCasual/Tutorial/'+$card+'.dds'
 $files[$name]='build/tutorial-assets/'+$name
}
$files['docs/WEAPON-TUTORIAL.md']='docs/WEAPON-TUTORIAL.md'
$names = @($files.Keys | Sort-Object)
foreach ($name in $names) {
    $source = Join-Path $repo $files[$name]
    if (!(Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Missing package input: $($files[$name]). Run build.cmd first if a DLL is missing."
    }
}
$sourceCommit = git -C $repo rev-parse HEAD
if($ReleaseCommit){if($ReleaseCommit -notmatch '^[0-9a-f]{40}$'){throw 'Invalid release commit'};$sourceCommit=$ReleaseCommit}
if ($LASTEXITCODE -ne 0) { throw 'Could not identify source commit.' }
$sourceChanges = @(git -C $repo status --porcelain --untracked-files=normal)
if ($LASTEXITCODE -ne 0) { throw 'Could not identify source working-tree state.' }
$manifest = [ordered]@{
    addon = 'Filthy Casual TTR'
    version = '1.8.0-beta.1'
    architecture = 'x86'
    channel = 'beta'
    features = @('robco-controls-editor','quest-touch-plus-3d','runtime-remapping','eight-profiles','mode-aware-providers','offline-editor','shared-robco-overlays','native-magazine-recovery','typed-ini-bindings','controller-profiles','70-input-sources','steam-frame-community-test','mouth-head-space-fix','separate-seat-memory','manual-recovered-magazine-chamber','native-glass-projection')
    tutorialDefault = 'off'
    tutorialCards = $cards.Count
    supportedTTWVR = '7EB72860D209D9B9BDD774607220EE63323367AA9FA525F6EB1B23184F30F91E'
    sourceCommit = "$sourceCommit".Trim()
    sourceDirty = (!$ReleaseCommit -and $sourceChanges.Count -gt 0)
    files = @($names | ForEach-Object {
        [ordered]@{
            path = $_
            sha256 = (Get-FileHash -LiteralPath (Join-Path $repo $files[$_]) -Algorithm SHA256).Hash
        }
    })
}
$manifestText = ($manifest | ConvertTo-Json -Depth 5) + "`n"
$utf8 = New-Object System.Text.UTF8Encoding($false)
Add-Type -AssemblyName System.IO.Compression
New-Item -ItemType Directory -Force -Path $out | Out-Null
$destination = Join-Path $out $OutputName
$temporary = Join-Path $out ('package-' + [guid]::NewGuid().ToString('N') + '.tmp')
# Fixed ordering and timestamps make identical inputs produce identical ZIPs
# under the same compression runtime. No file creation times enter the archive.
$timestamp = [DateTimeOffset]::new(2000, 1, 1, 0, 0, 0, [TimeSpan]::Zero)
$stream = $null
$archive = $null
try {
    $stream = [System.IO.File]::Open($temporary, [System.IO.FileMode]::CreateNew)
    $archive = [System.IO.Compression.ZipArchive]::new($stream, [System.IO.Compression.ZipArchiveMode]::Create, $true)
    foreach ($name in @(@($names) + 'manifest.json' | Sort-Object)) {
        $entry = $archive.CreateEntry($name, [System.IO.Compression.CompressionLevel]::Optimal)
        $entry.LastWriteTime = $timestamp
        $entryStream = $entry.Open()
        try {
            if ($name -eq 'manifest.json') {
                $bytes = $utf8.GetBytes($manifestText)
                $entryStream.Write($bytes, 0, $bytes.Length)
            } else {
                $inputStream = [System.IO.File]::OpenRead((Join-Path $repo $files[$name]))
                try { $inputStream.CopyTo($entryStream) } finally { $inputStream.Dispose() }
            }
        } finally { $entryStream.Dispose() }
    }
    $archive.Dispose()
    $archive = $null
    $stream.Dispose()
    $stream = $null
    Move-Item -LiteralPath $temporary -Destination $destination -Force
    [System.IO.File]::WriteAllText((Join-Path $out 'manifest.json'), $manifestText, $utf8)
} finally {
    if ($null -ne $archive) { $archive.Dispose() }
    if ($null -ne $stream) { $stream.Dispose() }
    # Only the explicitly created temporary file can be removed; never recurse.
    if (Test-Path -LiteralPath $temporary -PathType Leaf) { Remove-Item -LiteralPath $temporary -Force }
}
Write-Output $destination
