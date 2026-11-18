@echo off
rem Build chess.exe with MSYS2 UCRT64 GCC.
rem Output: build\chess.exe
setlocal
set "ROOT=%~dp0.."
set "UCRT=C:\msys64\ucrt64\bin"
set "GCC=%UCRT%\gcc.exe"
if not exist "%GCC%" (
  set "GCC=gcc"
) else (
  set "PATH=%UCRT%;%PATH%"
)

cd /d "%ROOT%"
if not exist build mkdir build

"%GCC%" -O2 -std=gnu89 -Wno-main -Wno-maybe-uninitialized ^
  -Iinclude -o build\chess.exe src\chess.c src\platform\win32\bgi_win32.c ^
  -lgdi32 -luser32 -lm
if errorlevel 1 (
  echo Build failed.
  exit /b 1
)
echo Built build\chess.exe
