@echo off
setlocal
cd /d "%~dp0"

echo ==============================================
echo       UNIVERSAL VR MOD LAUNCHER
echo ==============================================

:: Check for Administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [WARNING] Requesting Administrator Privileges...
    powershell -Command "Start-Process cmd -ArgumentList '/k \"\"%~dpnx0\"\"' -Verb RunAs"
    exit /b
)

echo [DEBUG] Running as Admin...
set INI_FILE="%~dp0VRModFramework\vr_config.ini"
if not exist %INI_FILE% (
    echo [ERROR] Could not find vr_config.ini!
    pause
    exit /b
)

for /f "tokens=1,2 delims==" %%a in ('findstr /I "^ProcessName=" %INI_FILE%') do ( set RAW_TARGET=%%b )
for /f "tokens=1" %%i in ("%RAW_TARGET%") do set TARGET_EXE=%%i

echo [INFO] Target Game: %TARGET_EXE%

set PID=
for /f "tokens=2" %%i in ('tasklist /NH ^| findstr /I "%TARGET_EXE%"') do (
    set PID=%%i
)

if "%PID%"=="" (
    echo [ERROR] Game is NOT running! Please start the game first, then run this script.
) else (
    echo [INFO] Game is running! Injecting VR Mod into PID: %PID%
    "%~dp0VRModFramework\build\Release\injector.exe" %PID% "%~dp0VRModFramework\build\Release\VRModFramework.dll"
    echo [SUCCESS] Injection Complete!
)
pause
 