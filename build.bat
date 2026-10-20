@echo off
setlocal

set "GPP="

REM 1. Explicit override: lets any developer point to their own install
if defined GPP_PATH if exist "%GPP_PATH%" set "GPP=%GPP_PATH%"

REM 2. Whatever g++ is already on PATH
if not defined GPP (
    for /f "delims=" %%G in ('where g++ 2^>nul') do (
        if not defined GPP set "GPP=%%G"
    )
)

REM 3. Common install locations
if not defined GPP (
    for %%P in (
        "C:\mingw64\bin\g++.exe"
        "C:\msys64\ucrt64\bin\g++.exe"
        "C:\msys64\mingw64\bin\g++.exe"
        "C:\MinGW\bin\g++.exe"
        "%USERPROFILE%\scoop\apps\mingw\current\bin\g++.exe"
    ) do (
        if not defined GPP if exist %%P set "GPP=%%~P"
    )
)

if not defined GPP (
    echo ERROR: Could not find g++.exe.
    echo Install MinGW-w64, add its bin folder to PATH, or set GPP_PATH to the full path of g++.exe.
    exit /b 1
)

echo Using compiler: %GPP%

REM Put the compiler's folder on PATH so runtime DLLs are found when running main.exe
for %%D in ("%GPP%") do set "PATH=%%~dpD;%PATH%"

cd /d "%~dp0"
if exist main.exe del /f /q main.exe

"%GPP%" -std=c++17 -g -pthread src\*.cpp -Iinclude -o main.exe
if errorlevel 1 exit /b 1

if %errorlevel% equ 0 (
    echo Build successful!
) else (
    echo Build failed.
)