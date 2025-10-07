@echo off
chcp 65001 >nul

echo ========================================
echo    Установка совместимого libpq через MSYS2
echo ========================================
echo.

set "MSYS2_PATH=C:\msys64"

if not exist "%MSYS2_PATH%\msys2_shell.cmd" (
    echo ❌ MSYS2 не найден!
    pause
    exit /b 1
)

echo Установка libpq через MSYS2...
echo.
echo Запуск MSYS2 MinGW64 для установки...
echo В открывшемся окне:
echo 1. Дождитесь завершения установки
echo 2. Закройте окно MSYS2
echo 3. Запустите build_program.bat снова
echo.

timeout /t 3 >nul

:: Запускаем MSYS2 и устанавливаем libpq
"%MSYS2_PATH%\msys2_shell.cmd" -mingw64 -defterm -c "pacman -S --needed --noconfirm mingw-w64-x86_64-postgresql"

echo.
echo ✅ libpq установлен через MSYS2
echo Теперь пути к библиотекам будут в:
echo   C:\msys64\mingw64\include
echo   C:\msys64\mingw64\lib
echo.
echo Запустите build_with_msys2_libpq.bat
echo.
pause