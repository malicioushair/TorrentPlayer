#Requires -Version 5.1
param(
    [string]$BuildDir = "build/distributive",
    [string]$PackageDir = "build/package/TorrentPlayer",
    [string]$MsiOutput = "build/package/TorrentPlayer-Windows-x64.msi",
    [string]$ProductVersion = "0.1.1",
    [string]$QtRoot = $env:QT_ROOT_DIR,
    [string]$BuildType = "Release",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Import-VcVars {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw "vswhere.exe was not found. Install Visual Studio 2022 with the C++ workload."
    }

    $vsInstallPath = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsInstallPath) {
        throw "Visual Studio C++ tools were not found."
    }

    $vcvars = Join-Path $vsInstallPath "VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path -LiteralPath $vcvars)) {
        throw "vcvars64.bat was not found at: $vcvars"
    }

    cmd.exe /c "`"$vcvars`" >nul && set" | ForEach-Object {
        if ($_ -match "^(?<key>[^=]+)=(?<value>.*)$") {
            Set-Item -Path "Env:$($Matches.key)" -Value $Matches.value
        }
    }
}

function Resolve-QtRoot {
    param([string]$RequestedQtRoot)

    if ($RequestedQtRoot -and (Test-Path -LiteralPath $RequestedQtRoot)) {
        return (Resolve-Path -LiteralPath $RequestedQtRoot).Path
    }

    $candidates = @(
        "C:\Qt\6.10.2\msvc2022_64"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $candidate "bin\Qt6Core.dll")) {
            return $candidate
        }
    }

    throw "Qt root was not found. Set QT_ROOT_DIR or pass -QtRoot."
}

function Copy-ConanRuntimeDlls {
    param(
        [string]$WorkspaceRoot,
        [string]$BuildDirectory,
        [string]$DestinationDirectory
    )

    $conanRun = Join-Path $BuildDirectory "conanrun.bat"
    if (-not (Test-Path -LiteralPath $conanRun)) {
        Write-Warning "conanrun.bat was not found at $conanRun. Skipping Conan DLL deployment."
        return
    }

    $conanPathOutput = & cmd.exe /d /v:on /c "call `"$conanRun`" >nul && echo !PATH!"
    $conanPath = $conanPathOutput | Select-Object -Last 1
    $copiedConanRuntime = 0

    foreach ($conanRuntimeDir in ($conanPath -split ';')) {
        if ($conanRuntimeDir -like "$env:USERPROFILE\.conan2\*") {
            Get-ChildItem -LiteralPath $conanRuntimeDir -Filter *.dll -File -ErrorAction SilentlyContinue |
                ForEach-Object {
                    Copy-Item -LiteralPath $_.FullName -Destination $DestinationDirectory -Force
                    $copiedConanRuntime++
                }
        }
    }

    if ($copiedConanRuntime -eq 0) {
        Write-Warning "No Conan shared runtime libraries were copied."
    } else {
        Write-Host "Copied $copiedConanRuntime Conan runtime DLL(s)."
    }
}

$workspaceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
Set-Location -LiteralPath $workspaceRoot

Import-VcVars
$qtRoot = Resolve-QtRoot -RequestedQtRoot $QtRoot

$wxsPath = Join-Path $workspaceRoot "packaging\windows\TorrentPlayer.wxs"
$exePath = Join-Path $PackageDir "bin\TorrentPlayer.exe"

if (-not $SkipBuild) {
    Write-Host "Installing Conan dependencies..."
    conan install . `
        -pr:b=default `
        -of $BuildDir `
        -s "build_type=$BuildType" `
        --build=missing

    Write-Host "Configuring CMake..."
    cmake -G Ninja -S . -B $BuildDir `
        "-DCMAKE_BUILD_TYPE=$BuildType" `
        "-DCMAKE_PREFIX_PATH=$qtRoot;$BuildDir" `
        "-DQT_HOST_PATH=$qtRoot" `
        "-DGLog_DIR=$BuildDir" `
        "-DLibtorrentRasterbar_DIR=$BuildDir"

    Write-Host "Building TorrentPlayer..."
    cmake --build $BuildDir --target TorrentPlayer --parallel
}

Write-Host "Installing deployable application to $PackageDir..."
if (Test-Path -LiteralPath $PackageDir) {
    Remove-Item -LiteralPath $PackageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null

cmake --install $BuildDir --config $BuildType --prefix $PackageDir

Copy-ConanRuntimeDlls -WorkspaceRoot $workspaceRoot -BuildDirectory $BuildDir -DestinationDirectory $PackageDir

if (-not (Test-Path -LiteralPath $exePath)) {
    throw "TorrentPlayer.exe was not found in the package directory: $exePath"
}

$iconPath = Join-Path $workspaceRoot "resources\windows\TorrentPlayer.ico"
if (-not (Test-Path -LiteralPath $iconPath)) {
    Write-Host "Generating Windows icon..."
    python (Join-Path $workspaceRoot "scripts\generate_windows_icon.py")
}

$msiDirectory = Split-Path -Parent $MsiOutput
if ($msiDirectory) {
    New-Item -ItemType Directory -Path $msiDirectory -Force | Out-Null
}

Write-Host "Building MSI..."
wix build `
    -ext WixToolset.UI.wixext `
    -arch x64 `
    -d "ProductVersion=$ProductVersion" `
    -bindpath "Payload=$PackageDir" `
    -out $MsiOutput `
    $wxsPath

Write-Host "Created installer: $MsiOutput"
