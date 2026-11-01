param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('x64', 'Win32')]
    [string]$Architecture = 'x64',
    [switch]$RaylibOnly
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build\$Architecture"
$cmakePath = $null
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (Test-Path -LiteralPath $vswherePath) {
    $visualStudioPath = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($visualStudioPath) {
        $bundledCmakePath = Join-Path $visualStudioPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
        if (Test-Path -LiteralPath $bundledCmakePath) {
            $cmakePath = $bundledCmakePath
        }
    }
}

if (-not $cmakePath) {
    $cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
    if (-not $cmakeCommand) {
        throw 'Install Visual Studio 2026 with Desktop development with C++ and CMake tools.'
    }
    $cmakePath = $cmakeCommand.Source
}
$cmakeHelp = & $cmakePath --help
if ($LASTEXITCODE -ne 0 -or -not ($cmakeHelp -match 'Visual Studio 18 2026')) {
    throw 'CMake 4.2 or later with the Visual Studio 2026 generator is required.'
}

& $cmakePath -S $projectRoot -B $buildDirectory -G 'Visual Studio 18 2026' -A $Architecture -DWOLCAN_BUILD_GAME=ON
if ($LASTEXITCODE -ne 0) {
    throw 'CMake configuration failed.'
}

if ($RaylibOnly) {
    & $cmakePath --build $buildDirectory --config $Configuration --target raylib --parallel 1
} else {
    & $cmakePath --build $buildDirectory --config $Configuration --parallel 1
}
if ($LASTEXITCODE -ne 0) {
    throw 'Compilation failed.'
}

if (-not $RaylibOnly) {
    Write-Output (Join-Path $buildDirectory "$Configuration\Wolcan.exe")
}
