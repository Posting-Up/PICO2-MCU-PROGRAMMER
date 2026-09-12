param([switch]$FetchDependencies)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    if ($FetchDependencies) {
        python tools/fetch_build_deps.py
        if ($LASTEXITCODE) { throw 'Dependency setup failed' }
    }
    $sdkPath = Join-Path $PSScriptRoot '.deps/pico-sdk'
    $compilerPath = Join-Path $PSScriptRoot '.deps/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin'
    cmake -S programmer -B build -G Ninja "-DPICO_SDK_PATH=$sdkPath" "-DPICO_TOOLCHAIN_PATH=$compilerPath" -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE) { throw 'CMake configuration failed' }
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vsPath = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vsPath) { throw 'Visual Studio C++ build tools are required for the native PIO assembler' }
    $vcvarsPath = Join-Path $vsPath 'VC/Auxiliary/Build/vcvars64.bat'
    & cmd /d /s /c "`"$vcvarsPath`" && cmake --build build --parallel 8"
    if ($LASTEXITCODE) { throw 'Firmware build failed' }
    Write-Output "UF2: $PSScriptRoot/build/atxmega192a3u_programmer.uf2"
} finally { Pop-Location }
