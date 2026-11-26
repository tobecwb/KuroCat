@echo off
setlocal
cd /d "%~dp0"

echo ==> Building KuroCat.exe (Windows native C++17 / Win32 + GDI+ + Raw Input)...

where cl >nul 2>nul
if %ERRORLEVEL%==0 (
    rc /nologo /fo resource.res resource.rc
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
    cl /nologo /std:c++17 /O2 /MT /EHsc /utf-8 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0601 /DWINVER=0x0601 ^
       /W3 src\main.cpp resource.res /Fe:KuroCat.exe ^
       /link /SUBSYSTEM:WINDOWS,6.01 /MANIFEST:EMBED /MANIFESTINPUT:app.manifest ^
       user32.lib gdi32.lib gdiplus.lib shell32.lib advapi32.lib winhttp.lib wtsapi32.lib ole32.lib
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
    del /q main.obj resource.res 2>nul
    echo ==> Built successfully with MSVC: %~dp0KuroCat.exe
    exit /b 0
)

where g++ >nul 2>nul
if %ERRORLEVEL%==0 (
    windres resource.rc -O coff -o resource.o
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
    g++ -std=c++17 -O2 -mwindows -municode -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -static ^
        src\main.cpp resource.o -o KuroCat.exe ^
        -luser32 -lgdi32 -lgdiplus -lshell32 -ladvapi32 -lwinhttp -lwtsapi32 -lole32
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
    del /q resource.o 2>nul
    echo ==> Built successfully with MinGW: %~dp0KuroCat.exe
    exit /b 0
)

echo [ERROR] Neither MSVC (cl.exe) nor MinGW (g++.exe) found in PATH.
echo Please run from "x64 Native Tools Command Prompt for VS" or install Visual Studio Build Tools.
exit /b 1
