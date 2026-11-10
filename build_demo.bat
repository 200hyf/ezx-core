@echo off
setlocal
cd /d "%~dp0"

echo ============================================
echo  ezx-core v2 demo build (MSVC x64, /MT, /utf-8)
echo ============================================

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found.
    echo         Install Visual Studio 2022/2026 with "Desktop development with C++".
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"

rem vswhere 找不到时（例如 VS 装在非默认目录、没注册进 Installer）回退到常见路径
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
    echo         vswhere found nothing; checked D:\VisualStudio2026 and Program Files defaults.
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
    echo         Install EasyX for this Visual Studio version first.
    exit /b 1
)

cl /nologo /std:c++17 /EHsc /MT /utf-8 /D_UNICODE /DUNICODE /I. ^
   /I"%VSDIR%\VC\Auxiliary\VS\include" ^
   easyx_core.cpp demo_main.cpp ^
   /Fe:demo.exe ^
   /link /LIBPATH:"%VSDIR%\VC\Auxiliary\VS\lib\x64"
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo [OK] Built: %CD%\demo.exe
echo      Run it from this folder so "res" can be found.
exit /b 0
