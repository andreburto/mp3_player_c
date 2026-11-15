@echo off
setlocal

rem Builds the MP3 Player application using the MSVC toolchain.
rem Produces bin\mp3player.exe.

set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%VCVARS%" (
    echo Could not find vcvars64.bat at "%VCVARS%".
    echo Edit build.bat to point at your Visual Studio installation.
    exit /b 1
)

if not exist bin mkdir bin

call "%VCVARS%" >nul
if errorlevel 1 (
    echo Failed to initialize the MSVC build environment.
    exit /b 1
)

cl /nologo /W4 /EHsc /D UNICODE /D _UNICODE /Fe:bin\mp3player.exe /Fo:bin\ ^
    src\mp3player.c src\player.c ^
    /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comdlg32.lib winmm.lib

if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Build succeeded: bin\mp3player.exe
endlocal
