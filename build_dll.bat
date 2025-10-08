@echo off
chcp 65001
echo ===============================================
echo    КОПИРОВАНИЕ ВСЕХ DLL (ПРОДВИНУТАЯ ВЕРСИЯ)
echo ===============================================

set EXE_DIR=build\bin
set MINGW_DIR=C:\msys64\mingw64\bin
set PG_DIR=C:\Program Files\PostgreSQL\17\bin

echo Создаю директорию для исполняемых файлов...
if not exist "%EXE_DIR%" mkdir "%EXE_DIR%"

echo.
echo Копирую ВСЕ MinGW DLL...
copy "%MINGW_DIR%\libgcc_s_seh-1.dll" "%EXE_DIR%" >nul 2>&1
copy "%MINGW_DIR%\libwinpthread-1.dll" "%EXE_DIR%" >nul 2>&1
copy "%MINGW_DIR%\libstdc++-6.dll" "%EXE_DIR%" >nul 2>&1

echo.
echo Копирую ВСЕ PostgreSQL DLL...
if exist "%PG_DIR%" (
    copy "%PG_DIR%\libpq.dll" "%EXE_DIR%" >nul 2>&1
    copy "%PG_DIR%\libintl-*.dll" "%EXE_DIR%" >nul 2>&1
    copy "%PG_DIR%\libcrypto-*.dll" "%EXE_DIR%" >nul 2>&1
    copy "%PG_DIR%\libssl-*.dll" "%EXE_DIR%" >nul 2>&1
    copy "%PG_DIR%\libiconv-*.dll" "%EXE_DIR%" >nul 2>&1
    copy "%PG_DIR%\zlib1.dll" "%EXE_DIR%" >nul 2>&1
    echo PostgreSQL DLL скопированы
) else (
    echo WARNING: PostgreSQL не найден по пути: %PG_DIR%
)

echo.
echo ===============================================
echo    ПРОВЕРКА СКОПИРОВАННЫХ DLL:
echo ===============================================
dir "%EXE_DIR%\*.dll"

echo.
echo ===============================================
echo    ПРОВЕРКА ЗАВИСИМОСТЕЙ:
echo ===============================================
if exist "%EXE_DIR%\StudentManagementSystem.exe" (
    echo Проверяем зависимости исполняемого файла...
    "%MINGW_DIR%\ldd.exe" "%EXE_DIR%\StudentManagementSystem.exe"
) else (
    echo Исполняемый файл не найден!
)

echo.
echo Готово! DLL скопированы в: %EXE_DIR%
echo Запустите: %EXE_DIR%\StudentManagementSystem.exe
pause