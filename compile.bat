
@echo off
setlocal

cd /d "%~dp0"

echo Compiling...

if not exist build mkdir build

g++ -Iinclude -I. src/compileropt.cpp src/error.cpp src/gc.cpp src/hash.cpp src/operator.cpp src/string.cpp src/state.cpp src/stdlib.cpp src/table.cpp src/lua.cpp main.cpp -o build/tinylc-main.exe

if errorlevel 1 (
    echo.
    echo [ERROR] Compilation failed.
    exit /b 1
)

echo Built build\tinylc-main.exe!
exit /b 0