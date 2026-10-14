@echo off
cd /d "%~dp0"
if exist main.exe del /f /q main.exe

C:\mingw64\bin\g++.exe -std=c++17 -g src/*.cpp -Iinclude -o main.exe

if %errorlevel% equ 0 (
    echo Build successful!
) else (
    echo Build failed.
)