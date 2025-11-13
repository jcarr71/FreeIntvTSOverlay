@echo off
REM Build Test Script for FreeIntvTSOverlay
REM Tests compilation with MinGW64 and Android NDK

setlocal enabledelayedexpansion

cd /d S:\VisualStudio\FreeIntvTSOverlay

echo.
echo ============================================
echo FreeIntvTSOverlay Build Test
echo ============================================
echo.

REM Check for required tools
echo [1/4] Checking build tools...
echo.

REM Check MinGW64
if exist C:\mingw64\bin\gcc.exe (
    echo [OK] MinGW64 found at C:\mingw64
    for /f "tokens=3" %%i in ('C:\mingw64\bin\gcc.exe --version ^| findstr /R "gcc"') do set GCC_VERSION=%%i
    echo     Version: !GCC_VERSION!
) else (
    echo [WARNING] MinGW64 not found at C:\mingw64
)

REM Check Android NDK
if exist C:\android\ndk (
    echo [OK] Android NDK found at C:\android\ndk
    REM Try to find NDK version
    if exist C:\android\ndk\source.properties (
        echo [OK] NDK source.properties found
    )
) else (
    echo [WARNING] Android NDK not found at C:\android\ndk
)

echo.
echo ============================================
echo [2/4] Building Windows (MinGW64) version...
echo ============================================
echo.

REM Set up MinGW environment
set PATH=C:\mingw64\bin;!PATH!

REM Try Windows build
echo Building with: make platform=win
make platform=win clean 2>nul >nul
if !ERRORLEVEL! EQU 0 (
    echo Clean successful
) else (
    echo Clean skipped (first build)
)

make platform=win
if !ERRORLEVEL! EQU 0 (
    echo.
    echo [OK] Windows build successful!
    if exist freeintvtsoverlay_libretro.dll (
        echo [OK] Output file: freeintvtsoverlay_libretro.dll
        for /f "%%A in ('wc -c < freeintvtsoverlay_libretro.dll') do set FILE_SIZE=%%A
        echo     File size: !FILE_SIZE! bytes
    )
) else (
    echo [ERROR] Windows build failed!
    exit /b 1
)

echo.
echo ============================================
echo [3/4] Building Android (NDK) version...
echo ============================================
echo.

if exist C:\android\ndk (
    REM Set up NDK environment
    set NDK_PATH=C:\android\ndk
    set PATH=!NDK_PATH!\toolchains\llvm\prebuilt\windows-x86_64\bin;!PATH!
    
    echo Building with: ndk-build -C jni/
    cd /d S:\VisualStudio\FreeIntvTSOverlay
    
    call ndk-build -C jni/ NDK_PROJECT_PATH=. 2>&1
    
    if !ERRORLEVEL! EQU 0 (
        echo.
        echo [OK] Android build successful!
        REM Check for Android build outputs
        if exist libs\arm64-v8a\libfreeintvtsoverlay_libretro.so (
            echo [OK] arm64-v8a library built
        )
        if exist libs\armeabi-v7a\libfreeintvtsoverlay_libretro.so (
            echo [OK] armeabi-v7a library built
        )
    ) else (
        echo [WARNING] Android build had issues (may be expected if NDK not fully configured)
    )
) else (
    echo [SKIP] Android NDK not found, skipping Android build
)

echo.
echo ============================================
echo [4/4] Verification
echo ============================================
echo.

REM Check for compilation errors
echo Checking for compilation errors...
if exist freeintvtsoverlay_libretro.dll (
    echo [OK] Windows DLL present
) else (
    echo [ERROR] Windows DLL not found!
)

echo.
echo ============================================
echo Build Test Complete
echo ============================================
echo.
echo Next steps:
echo 1. Copy freeintvtsoverlay_libretro.dll to RetroArch\cores\
echo 2. Place exec.bin and grom.bin in RetroArch\system\
echo 3. Load a game ROM in RetroArch
echo 4. Test fullscreen toggle button (Button 3)
echo 5. Verify auto-hide control strip functionality
echo.

endlocal
