@echo off
cmake -B build -S .
cmake --build build --config Release
echo.
echo done: build\Release\memLyrics.exe
pause
