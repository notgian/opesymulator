@echo off
cd /d "%~dp0"
if exist main.exe del /f /q main.exe

C:\msys64\ucrt64\bin\g++.exe -std=c++17 -g -pthread src/*.cpp -Iinclude -o main.exe

if %errorlevel% equ 0 (
    echo Build successful!
) else (
    echo Build failed.
)