Write-Host "Student Record System - Building with CMake & Vcpkg" -ForegroundColor Cyan
Write-Host "=" * 60

# Check if vcpkg exists
if (-not (Test-Path "vcpkg/vcpkg.exe")) {
    Write-Host "Error: vcpkg not found. Please run local setup first." -ForegroundColor Red
    exit 1
}

# Find CMake
$cmakeExe = "cmake"
if (-not (Get-Command "cmake" -ErrorAction SilentlyContinue)) {
    $localCmake = Get-ChildItem -Path "vcpkg/downloads/tools" -Filter "cmake.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($localCmake) {
        $cmakeExe = $localCmake.FullName
        Write-Host "Using local CMake: $cmakeExe" -ForegroundColor Gray
    } else {
        Write-Host "Error: CMake not found in PATH or vcpkg tools. Please install CMake." -ForegroundColor Red
        exit 1
    }
}

# Configure
Write-Host "Configuring CMake..." -ForegroundColor Green
$toolchain = "vcpkg/scripts/buildsystems/vcpkg.cmake"
& $cmakeExe -B build -S . "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DVCPKG_TARGET_TRIPLET=x64-windows"
if ($LASTEXITCODE -ne 0) { exit 1 }

# Build
Write-Host "Building project..." -ForegroundColor Green
& $cmakeExe --build build --config Release
if ($LASTEXITCODE -ne 0) { exit 1 }

# Install/Organize (Move to bin/)
Write-Host "Moving executables to bin/..." -ForegroundColor Green
New-Item -ItemType Directory -Path "bin" -Force | Out-Null
Copy-Item "build\Release\*.exe" -Destination "bin" -Force
Copy-Item "config.json" -Destination "bin" -Force

Write-Host "Build Complete!" -ForegroundColor Green
