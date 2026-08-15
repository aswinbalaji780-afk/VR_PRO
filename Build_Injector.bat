@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo =========================================
echo       BUILDING INJECTOR EXECUTABLE
echo =========================================

:: Create output directory if it doesn't exist
if not exist "VRModFramework\build\Release" (
    mkdir "VRModFramework\build\Release"
)

:: Find Visual Studio installation path using vswhere
set VS_PATH=
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do (
    set "VS_PATH=%%i"
)

if "!VS_PATH!"=="" (
    echo [ERROR] Could not auto-detect Visual Studio installation.
    echo Please run this script from "Developer Command Prompt for Visual Studio".
    pause
    exit /b 1
)

echo [INFO] Found Visual Studio at: !VS_PATH!
set "VCVARS=!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat"

if not exist "!VCVARS!" (
    echo [ERROR] Could not find vcvars64.bat at !VCVARS!
    pause
    exit /b 1
)

echo [INFO] Initializing MSVC x64 build environment...
call "!VCVARS!" >nul 2>&1

echo [INFO] Compiling injector.cpp ...
cl.exe /EHsc /O2 /Fe:"VRModFramework\build\Release\injector.exe" injector.cpp user32.lib kernel32.lib

if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed!
    pause
    exit /b %errorlevel%
)

if exist "injector.obj" del "injector.obj"

echo.
echo [SUCCESS] VRModFramework\build\Release\injector.exe compiled successfully!

if exist "Universal_VR_Mod" (
    copy /Y "VRModFramework\build\Release\injector.exe" "Universal_VR_Mod\" >nul
    echo [INFO] Copied injector.exe to Universal_VR_Mod\ directory.
)

pause
