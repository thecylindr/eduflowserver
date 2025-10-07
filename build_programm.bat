@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ========================================
echo    Сборка Student Management System
echo    (MSYS2 + MinGW64) - Кроссплатформенная
echo ========================================
echo.

:: Проверяем PostgreSQL
set "PG_PATH=C:\Program Files\PostgreSQL\17"
if not exist "%PG_PATH%\include\libpq-fe.h" (
    echo ❌ ОШИБКА: PostgreSQL 17 не найден!
    echo.
    echo Установите PostgreSQL 17 в C:\Program Files\PostgreSQL\17
    echo Скачайте: https://www.postgresql.org/download/windows/
    echo.
    pause
    exit /b 1
)
echo ✅ PostgreSQL 17 найден

:: Проверяем MSYS2
set "MSYS2_PATH=C:\msys64"
set "MINGW64_PATH=%MSYS2_PATH%\mingw64\bin"

if not exist "%MSYS2_PATH%\msys2_shell.cmd" (
    echo ❌ MSYS2 не найден!
    echo Установите MSYS2: https://www.msys2.org/
    pause
    exit /b 1
)
echo ✅ MSYS2 найден: %MSYS2_PATH%

:: Проверяем MinGW64 в MSYS2
echo Проверка MinGW64 в MSYS2...
if not exist "%MINGW64_PATH%\g++.exe" (
    echo ❌ MinGW64 не найден в MSYS2!
    echo.
    echo Установка MinGW64 в MSYS2...
    echo.
    
    :: Создаем скрипт для установки MinGW64 в MSYS2
    echo Создание установочного скрипта...
    (
        echo @echo off
        echo chcp 65001 ^>nul
        echo echo Установка MinGW64 в MSYS2...
        echo echo.
        echo echo Запуск MSYS2 для установки компилятора...
        echo echo Это может занять несколько минут...
        echo echo.
        echo "%MSYS2_PATH%\msys2_shell.cmd" -mingw64 -defterm -c "pacman -S --needed --noconfirm mingw-w64-x86_64-gcc"
        echo echo.
        echo echo ✅ MinGW64 установлен!
        echo echo Закройте это окно и запустите build_program.bat снова
        echo pause
    ) > install_mingw64.bat
    
    echo Запуск установки MinGW64...
    echo.
    echo В открывшемся окне MSYS2:
    echo 1. Дождитесь завершения установки
    echo 2. Закройте окно MSYS2
    echo 3. Запустите build_program.bat снова
    echo.
    timeout /t 3 >nul
    start install_mingw64.bat
    exit /b 0
)

echo ✅ MinGW64 найден: %MINGW64_PATH%
echo.
echo Проверка компилятора...
"%MINGW64_PATH%\g++" --version >nul 2>&1
if !errorlevel! neq 0 (
    echo ❌ Ошибка: GCC не работает!
    echo.
    echo Переустановите MinGW64 в MSYS2:
    echo 1. Запустите MSYS2 MinGW 64-bit
    echo 2. Выполните: pacman -S mingw-w64-x86_64-gcc
    echo.
    pause
    exit /b 1
)

"%MINGW64_PATH%\g++" --version | findstr "g++"
echo ✅ Компилятор готов к работе

:: Добавляем MSYS2 MinGW64 в PATH для текущей сессии
set "PATH=%MINGW64_PATH%;%PATH%"

:: Удаляем временный установочный скрипт если он существует
if exist "install_mingw64.bat" del "install_mingw64.bat"

:: Создаем папку для сборки
mkdir build 2>nul
cd build

echo.
echo ========================================
echo    КОМПИЛЯЦИЯ ПРОГРАММЫ
echo ========================================
echo.

:: Компилируем все файлы с определением _WIN32 для Windows
echo [1/5] Компиляция main.cpp...
g++ -c -I"%PG_PATH%\include" -I"../src" -std=c++17 -O2 -D_WIN32 ../src/main.cpp
if !errorlevel! neq 0 goto :compile_error

echo [2/5] Компиляция DatabaseService.cpp...
g++ -c -I"%PG_PATH%\include" -I"../src" -std=c++17 -O2 -D_WIN32 ../src/DatabaseService.cpp
if !errorlevel! neq 0 goto :compile_error

echo [3/5] Компиляция ConfigManager.cpp...
g++ -c -I"%PG_PATH%\include" -I"../src" -std=c++17 -O2 -D_WIN32 ../src/ConfigManager.cpp
if !errorlevel! neq 0 goto :compile_error

echo [4/5] Компиляция ApiService.cpp...
g++ -c -I"%PG_PATH%\include" -I"../src" -std=c++17 -O2 -D_WIN32 ../src/ApiService.cpp
if !errorlevel! neq 0 goto :compile_error

echo [5/5] Линковка исполняемого файла...
g++ *.o -L"%PG_PATH%\lib" -lpq -lws2_32 -lwsock32 -o StudentManagementSystem.exe
if !errorlevel! neq 0 goto :link_error

:: Копирование DLL
echo.
echo Копирование библиотек...
copy "%PG_PATH%\bin\libpq.dll" . >nul 2>&1
echo ✅ libpq.dll скопирована

:: Копируем необходимые DLL из MSYS2 MinGW64
copy "%MINGW64_PATH%\libstdc++-6.dll" . >nul 2>&1
echo ✅ libstdc++-6.dll скопирована

copy "%MINGW64_PATH%\libgcc_s_seh-1.dll" . >nul 2>&1
echo ✅ libgcc_s_seh-1.dll скопирована

copy "%MINGW64_PATH%\libwinpthread-1.dll" . >nul 2>&1
echo ✅ libwinpthread-1.dll скопирована

echo.
echo ========================================
echo    ✅ СБОРКА УСПЕШНО ЗАВЕРШЕНА!
echo ========================================
echo.
echo 📁 Созданные файлы:
dir /B *.exe *.dll 2>nul
echo.
echo 🚀 Запуск приложения...
echo.
timeout /t 3 >nul

StudentManagementSystem.exe
goto :end

:compile_error
echo.
echo ❌ ОШИБКА КОМПИЛЯЦИИ!
echo Проверьте исходные файлы в папке src\
goto :error

:link_error
echo.
echo ❌ ОШИБКА ЛИНКОВКИ!
echo Проверьте пути к библиотекам PostgreSQL
echo Убедитесь что библиотеки ws2_32 и wsock32 доступны
goto :error

:error
echo.
echo Возможные решения:
echo 1. Проверьте наличие всех исходных файлов в src\
echo 2. Убедитесь что PostgreSQL установлен правильно
echo 3. Проверьте что MinGW64 установлен в MSYS2
echo 4. Убедитесь что библиотеки ws2_32 и wsock32 доступны
echo.
pause
exit /b 1

:end
cd ..
pause