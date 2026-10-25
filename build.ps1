param([switch]$UniversalLoader)

$ErrorActionPreference = 'Stop'

Push-Location $PSScriptRoot

try {

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'

    $install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

    if (!$install) { throw 'Visual Studio C++ build tools are required.' }

    $cmake = Join-Path $install 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'

    $ctest = Join-Path (Split-Path $cmake) 'ctest.exe'

    $bundleProxy = if ($UniversalLoader) { 'OFF' } else { 'ON' }
    & $cmake -S . -B build -G 'Visual Studio 18 2026' -A x64 "-DJISH_BUNDLE_PROXY=$bundleProxy"

    if ($LASTEXITCODE) { throw 'Configure failed.' }

    & $cmake --build build --config Release --parallel 1 -- /p:VcpkgEnabled=false /nr:false

    if ($LASTEXITCODE) { throw 'Build failed.' }

    & $ctest --test-dir build -C Release --output-on-failure

    if ($LASTEXITCODE) { throw 'Checks failed.' }

    & $cmake --install build --config Release --prefix release

    if ($LASTEXITCODE) { throw 'Install staging failed.' }
    if ($UniversalLoader) {
        # Remove only an older staged proxy inside this source checkout.
        $stagedProxy = Join-Path $PSScriptRoot 'release/dinput8.dll'
        if (Test-Path -LiteralPath $stagedProxy) { Remove-Item -LiteralPath $stagedProxy }
        Write-Host 'Universal loader package: keep your existing game-folder dinput8.dll.'
    }
    $oldPreset = Join-Path $PSScriptRoot 'release/scripts/Custom/presets/recommended.ini'
    if (Test-Path -LiteralPath $oldPreset) { Remove-Item -LiteralPath $oldPreset }
    New-Item -ItemType Directory -Force release/scripts/Custom/FirstPerson/logs | Out-Null

    Copy-Item README.md,SETTINGS.md -Destination release
    New-Item -ItemType Directory -Force release/docs | Out-Null
    Copy-Item docs/* -Destination release/docs -Recurse -Force

    Copy-Item vendor/LICENSE -Destination release/LICENSE.txt

    New-Item -ItemType Directory -Force release/licenses | Out-Null

    Copy-Item vendor/External/detours/LICENSE.txt -Destination release/licenses/Detours-LICENSE.txt

    Copy-Item vendor/External/rapidjson/LICENSE.txt -Destination release/licenses/RapidJSON-LICENSE.txt

    Copy-Item LAUNCHER_INSTALL.md -Destination release

    $legacyDll = Join-Path $PSScriptRoot 'release/JishsFirstPersonMod.dll'

    if (Test-Path -LiteralPath $legacyDll) { Remove-Item -LiteralPath $legacyDll }

    Get-FileHash release/scripts/Custom/JishsFirstPersonMod.dll

} finally { Pop-Location }
