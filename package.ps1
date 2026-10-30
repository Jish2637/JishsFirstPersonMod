param([string]$ModVersion = "1.36.7-studio-test1", [string]$ReSkateVersion = "2.1.3-animator-studio-1.0.101", [switch]$UniversalLoader)

$ErrorActionPreference = 'Stop'

$releaseName = "JishsFirstPersonMod-$ModVersion-reskate-$ReSkateVersion"

if ($UniversalLoader) { $releaseName += "-universal-loader" }

$releasePath = Join-Path $PSScriptRoot ($releaseName + '.zip')

Push-Location $PSScriptRoot

try {

    if (!(Test-Path release/scripts/Custom/FirstPerson/updater/FirstPersonUpdater.exe) -or
        (!$UniversalLoader -and !(Test-Path release/dinput8.dll)) -or

        !(Test-Path release/scripts/Custom/JishsFirstPersonMod.dll) -or

        !(Test-Path release/scripts/Custom/jishs_first_person_mod.lua) -or

        !(Test-Path release/scripts/Custom/FirstPerson/presets/bundled/recommended.ini)) { throw 'Run build.ps1 before packaging.' }

    $legacyDll = Join-Path $PSScriptRoot 'release/JishsFirstPersonMod.dll'

    if (Test-Path -LiteralPath $legacyDll) { Remove-Item -LiteralPath $legacyDll }

    $licensePaths = @('vendor/LICENSE','vendor/External/detours/LICENSE.txt','vendor/External/rapidjson/LICENSE.txt','vendor/External/imgui/LICENSE.txt','vendor/External/miniz/LICENSE.txt')

    $licenseText = ($licensePaths | ForEach-Object { "--- $_ ---`n" + [IO.File]::ReadAllText((Join-Path $PSScriptRoot $_)) }) -join "`n`n"

    [IO.File]::WriteAllText((Join-Path $PSScriptRoot 'release/LICENSE.txt'),$licenseText,[Text.UTF8Encoding]::new($false))

    $clientPaths = @('release/Mods','release/scripts','release/LICENSE.txt','release/README.md')
    if (!$UniversalLoader) { $clientPaths += 'release/dinput8.dll' }
    Compress-Archive -Path $clientPaths -DestinationPath $releasePath -Force

    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $sourcePath = Join-Path $PSScriptRoot ($releaseName + '-source.zip')

    $stream = [IO.File]::Open($sourcePath, [IO.FileMode]::Create)

    $archive = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create)

    try {

        foreach ($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Recurse -File)) {

            $relative = [IO.Path]::GetRelativePath($PSScriptRoot, $file.FullName).Replace('\','/')

            if ($relative -match '^(build[^/]*|release|\.hook-tools|\.git)/|(^|/)__pycache__/|^[^/]*\.log$|\.zip$|\.pyc$') { continue }

            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $relative, [IO.Compression.CompressionLevel]::Optimal) | Out-Null

        }

    } finally { $archive.Dispose(); $stream.Dispose() }

    Get-FileHash -LiteralPath $releasePath,$sourcePath

} finally { Pop-Location }
