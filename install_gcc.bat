@echo off
chcp 65001 >nul

echo ========================================
echo    Установка GCC (MinGW-w64) на Windows
echo ========================================
echo.

echo Скачивание MSYS2...
powershell -Command "Invoke-WebRequest -Uri 'https://github.com/msys2/msys2-installer/releases/download/2023-03-18/msys2-x86_64-20230318.exe' -OutFile 'msys2.exe'"

echo Установка MSYS2...
start /wait msys2.exe

echo Ожидание завершения установки...
timeout /t 10

echo Запуск MSYS2 для установки GCC...
C:\msys64\msys2_shell.cmd -defterm -here -no-start -mingw64 -c "pacman -Syu --noconfirm && pacman -S --noconfirm mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make"

echo.
echo ========================================
echo ✅ GCC установлен!
echo ========================================
echo.
echo Добавьте в PATH:
echo C:\msys64\mingw64\bin
echo.
echo Проверка:
gcc --version
g++ --version
echo.
pause