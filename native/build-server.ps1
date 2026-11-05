param(
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$QtTools = "C:\Qt\Tools"
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
if (!$VcpkgRoot -or !(Test-Path (Join-Path $VcpkgRoot "vcpkg.exe"))) {
    throw "Bootstrap vcpkg first and set VCPKG_ROOT or pass -VcpkgRoot."
}
$cmake = Join-Path $QtTools "CMake_64\bin\cmake.exe"
$ninja = Join-Path $QtTools "Ninja\ninja.exe"
$mingw = Join-Path $QtTools "mingw1310_64\bin"
foreach ($file in @($cmake, $ninja, (Join-Path $mingw "g++.exe"))) {
    if (!(Test-Path -LiteralPath $file)) { throw "Missing Qt tool: $file" }
}
$build = Join-Path $root "build\native-server"
$oldPath = $env:Path
try {
    $env:Path = "$mingw;$oldPath"
    & $cmake -S (Join-Path $root "cpp") -B $build -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot/scripts/buildsystems/vcpkg.cmake" "-DVCPKG_MANIFEST_DIR=$root" "-DVCPKG_TARGET_TRIPLET=x64-mingw-static" "-DVCPKG_HOST_TRIPLET=x64-mingw-static" "-DCMAKE_EXE_LINKER_FLAGS=-static"
    if ($LASTEXITCODE -ne 0) { throw "Server configuration failed." }
    & $cmake --build $build --parallel 2
    if ($LASTEXITCODE -ne 0) { throw "Server build failed." }
}
finally { $env:Path = $oldPath }
