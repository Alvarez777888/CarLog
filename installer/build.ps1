param(
    [string]$QtBin = "C:\Qt\6.11.1\mingw_64\bin",
    [string]$MinGwBin = "C:\Qt\Tools\mingw1310_64\bin",
    [string]$IsccPath = "",
    [string]$ServerUrl = "",
    [switch]$LocalOnly
)
$ErrorActionPreference = "Stop"
$projectRoot = Split-Path $PSScriptRoot -Parent
if ($projectRoot -match '[^\x00-\x7F]') {
    throw "Extract the project to an ASCII path before packaging; MinGW windres cannot read the icon otherwise."
}
$clientBuild = Join-Path $projectRoot "build\installer-client"
$stage = Join-Path $projectRoot "build\installer-staging"
function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Command failed ($LASTEXITCODE): $Program" }
}
if (!$IsccPath) {
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) { $IsccPath = $command.Source }
    $bases = @([Environment]::GetEnvironmentVariable("ProgramFiles(x86)"), $env:ProgramFiles, (Join-Path $env:LOCALAPPDATA "Programs"))
    foreach ($base in $bases) {
        if (!$base) { continue }
        foreach ($version in @("7", "6")) {
            $candidate = Join-Path $base "Inno Setup $version\ISCC.exe"
            if (!$IsccPath -and (Test-Path $candidate)) { $IsccPath = $candidate }
        }
    }
}
if (!$IsccPath -or !(Test-Path $IsccPath)) {
    throw "Install Inno Setup or pass -IsccPath with the full path to ISCC.exe."
}
foreach ($file in @((Join-Path $QtBin "qmake.exe"), (Join-Path $QtBin "windeployqt.exe"), (Join-Path $MinGwBin "mingw32-make.exe"))) {
    if (!(Test-Path $file)) { throw "Missing tool: $file" }
}
if (!$ServerUrl -and !$LocalOnly) {
    throw 'Pass -ServerUrl with the shared HTTPS API address. Use -LocalOnly only for testing on the server PC.'
}
if ($ServerUrl -and $LocalOnly) { throw "Choose -ServerUrl or -LocalOnly." }
if ($ServerUrl) {
    $uri = $null
    if (![Uri]::TryCreate($ServerUrl, [UriKind]::Absolute, [ref]$uri) -or
        $uri.Scheme -ne "https" -or $uri.IsLoopback -or $uri.UserInfo -or $uri.Query -or $uri.Fragment -or
        ($uri.AbsolutePath -ne "/")) {
        throw "ServerUrl must be a non-loopback HTTPS server origin."
    }
    $health = Invoke-RestMethod ($ServerUrl.TrimEnd('/') + '/api/health') -TimeoutSec 15
    if ($health.status -ne "ok" -or @($health.failed_modules).Count -ne 0) {
        throw "The shared CarLog API did not report healthy. Check the server and its HTTPS address."
    }
}
$previousPath = $env:Path
try {
    $env:Path = "$QtBin;$MinGwBin;$previousPath"
    New-Item -ItemType Directory -Force $clientBuild | Out-Null
    Push-Location $clientBuild
    try {
        Invoke-Checked -Program (Join-Path $QtBin "qmake.exe") -Arguments @("-spec", "win32-g++", (Join-Path $projectRoot "client\client.pro"))
        Invoke-Checked -Program (Join-Path $MinGwBin "mingw32-make.exe") -Arguments @("release", "-j2")
    }
    finally { Pop-Location }
    # This folder is generated exclusively for the package.
    if (Test-Path $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
    New-Item -ItemType Directory -Force $stage | Out-Null
    Copy-Item (Join-Path $clientBuild "release\carlog.exe") $stage
    Copy-Item (Join-Path $projectRoot "client\resources\carlog.ico") $stage
    Invoke-Checked -Program (Join-Path $QtBin "windeployqt.exe") -Arguments @("--release", "--compiler-runtime", "--dir", $stage, (Join-Path $stage "carlog.exe"))
    $ini = Join-Path $stage "carlog.ini"
    if ($ServerUrl) {
        $content = "[server]" + [Environment]::NewLine + "url=" + $ServerUrl.TrimEnd('/') + [Environment]::NewLine
        [IO.File]::WriteAllText($ini, $content, (New-Object Text.UTF8Encoding($false)))
    }
    else { Copy-Item (Join-Path $projectRoot "client\carlog.ini.example") $ini }
    foreach ($required in @("carlog.exe", "carlog.ico", "carlog.ini", "Qt6Core.dll", "Qt6Widgets.dll", "Qt6Network.dll", "platforms\qwindows.dll", "tls\qschannelbackend.dll", "libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll")) {
        if (!(Test-Path (Join-Path $stage $required))) { throw "Missing package file: $required" }
    }
    Invoke-Checked -Program $IsccPath -Arguments @("/DStageDir=$stage", (Join-Path $PSScriptRoot "CarLog.iss"))
    Write-Host ("Installer ready: " + (Join-Path $projectRoot "build\installer\CarLogSetup.exe"))
}
finally { $env:Path = $previousPath }
