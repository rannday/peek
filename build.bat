@echo off
setlocal
set SRC=src\main.c src\utils\time.c src\utils\sys.c
set CFLAGS=-std=c11 -Wall -Wextra -D_CRT_SECURE_NO_WARNINGS -DVERSION=\"0.1.0\"

if /I "%1"=="debug" (
  zig cc %CFLAGS% -O0 -g -o peek.exe %SRC%
) else (
  zig cc %CFLAGS% -O2 -o peek.exe %SRC%
  if exist peek.exe zig strip peek.exe >nul 2>&1
)
echo Built peek.exe
