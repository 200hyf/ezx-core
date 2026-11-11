@echo off
setlocal
cd /d "%~dp0"

echo ============================================
echo  ezx-core v2.1 sprite demo build (MSVC x64, /MT, /utf-8)
echo ============================================

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSDIR="
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
)

rem Fallback paths when vswhere cannot find VS (custom install location, not registered).
if not defined VSDIR (
    for %%p in (
        "D:\VisualStudio2026"
        "D:\VisualStudio2022"
        "C:\Program Files\Microsoft Visual Studio\2022\Community"
        "C:\Program Files\Microsoft Visual Studio\2022\Professional"
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
        "C:\Program Files\Microsoft Visual Studio\18\Community"
    ) do (
        if not defined VSDIR if exist "%%~p\VC\Auxiliary\Build\vcvars64.bat" set "VSDIR=%%~p"
    )
)

if not defined VSDIR (
    echo [ERROR] Visual Studio C++ tools not found.
    exit /b 1
)

echo Using Visual Studio: %VSDIR%
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed.
    exit /b 1
)

if not exist "%VSDIR%\VC\Auxiliary\VS\lib\x64\EasyXw.lib" (
    echo [ERROR] EasyX not found in:
    echo         %VSDIR%\VC\Auxiliary\VS\lib\x64
    exit /b 1
)

cl /nologo /std:c++17 /EHsc /MT /utf-8 /D_UNICODE /DUNICODE /I. ^
   /I"%VSDIR%\VC\Auxiliary\VS\include" ^
   easyx_core.cpp examples\sprite_demo.cpp ^
   /Fe:sprite_demo.exe ^
   /link /LIBPATH:"%VSDIR%\VC\Auxiliary\VS\lib\x64"
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo [OK] Built: %CD%\sprite_demo.exe
echo      Run:  sprite_demo.exe
echo      Test: sprite_demo.exe --selftest 120
exit /b 0
