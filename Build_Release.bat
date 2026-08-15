@echo off
setlocal enabledelayedexpansion

echo =========================================
echo       PACKAGING MOD FOR DISTRIBUTION
echo =========================================

set DIST_DIR=Universal_VR_Mod
if exist "%DIST_DIR%" (
    rmdir /S /Q "%DIST_DIR%"
)
mkdir "%DIST_DIR%"

echo Copying files...
copy "VRModFramework\build\Release\VRModFramework.dll" "%DIST_DIR%\"
copy "VRModFramework\build\Release\injector.exe" "%DIST_DIR%\"
copy "VRModFramework\vr_config.ini" "%DIST_DIR%\"

:: Create a modified Launch_VR.bat that expects all files in the same folder
(
echo @echo off
echo setlocal
echo cd /d "%%~dp0"
echo.
echo :: Check for Administrator privileges
echo net session ^>nul 2^>^&1
echo if %%errorLevel%% neq 0 ^(
echo     echo [WARNING] Requesting Administrator Privileges...
echo     powershell -Command "Start-Process cmd -ArgumentList '/k \"\"%%~dpnx0\"\"' -Verb RunAs"
echo     exit /b
echo ^)
echo.
echo echo [DEBUG] Running as Admin...
echo set INI_FILE="%%~dp0vr_config.ini"
echo if not exist %%INI_FILE%% ^(
echo     echo [ERROR] Could not find vr_config.ini!
echo     pause
echo     exit /b
echo ^)
echo.
echo for /f "tokens=1,2 delims==" %%%%a in ^('findstr /I "^ProcessName=" %%INI_FILE%%'^) do ^( set RAW_TARGET=%%%%b ^)
echo for /f "tokens=1" %%%%i in ^("%%RAW_TARGET%%"^) do set TARGET_EXE=%%%%i
echo.
echo echo [INFO] Target Game: %%TARGET_EXE%%
echo.
echo set PID=
echo for /f "tokens=2" %%%%i in ^('tasklist /NH ^^^| findstr /I "%%TARGET_EXE%%"'^) do ^(
echo     set PID=%%%%i
echo ^)
echo.
echo if "%%PID%%"=="" ^(
echo     echo [ERROR] Game is NOT running! Please start the game first, then run this script.
echo ^) else ^(
echo     echo [INFO] Game is running! Injecting VR Mod into PID: %%PID%%
echo     "%%~dp0injector.exe" %%PID%% "%%~dp0VRModFramework.dll"
echo     echo [SUCCESS] Injection Complete!
echo ^)
echo pause
) > "%DIST_DIR%\Launch_VR.bat"

echo.
echo Zipping the release package...
powershell -Command "Compress-Archive -Path '%DIST_DIR%' -DestinationPath '%DIST_DIR%.zip' -Force"

echo.
echo [SUCCESS] Packaging complete! 
echo A new ZIP file named "%DIST_DIR%.zip" has been created.
echo You can upload this directly to GitHub Releases!
pause
