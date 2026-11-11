param([ValidatePattern('^[A-Za-z0-9._-]+\.zip$')][string]$OutputName = 'Filthy-Casual-TTR-1.5.1-beta.zip')
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
    'docs/PHYSICAL_PARTS.md' = 'docs/PHYSICAL_PARTS.md'
    'README.md' = 'README.md'
    'docs/INSTALL.md' = 'docs/INSTALL.md'
    'docs/assets/terminal-banner.svg' = 'docs/assets/terminal-banner.svg'
    'meta.ini' = 'meta.ini'
    'THIRD_PARTY.md' = 'THIRD_PARTY.md'
    'third_party/openxr/LICENSES/Apache-2.0.txt' = 'third_party/openxr/LICENSES/Apache-2.0.txt'
    'third_party/openxr/LICENSES/MIT.txt' = 'third_party/openxr/LICENSES/MIT.txt'
}
$names = @($files.Keys | Sort-Object)
foreach ($name in $names) {
    $source = Join-Path $repo $files[$name]
    if (!(Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Missing package input: $($files[$name]). Run build.cmd first if a DLL is missing."
    }
}
$sourceCommit = git -C $repo rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Could not identify source commit.' }
$sourceChanges = @(git -C $repo status --porcelain --untracked-files=normal)
if ($LASTEXITCODE -ne 0) { throw 'Could not identify source working-tree state.' }
$manifest = [ordered]@{
    addon = 'Filthy Casual TTR'
    version = '1.5.1-beta'
    architecture = 'x86'
    supportedTTWVR = 'C9D047D4199C0458281D341AC9DC438A7C928999E19CDAD6F327F8A439A7F9EB'
    sourceCommit = "$sourceCommit".Trim()
    sourceDirty = ($sourceChanges.Count -gt 0)
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
