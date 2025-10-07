@echo off
chcp 65001 >nul

echo Проверка переменной PATH:
echo.
echo %PATH% | findstr /C:"CMake" >nul && echo ✅ CMake в PATH || echo ❌ CMake не в PATH
echo %PATH% | findstr /C:"mingw" >nul && echo ✅ MinGW в PATH || echo ❌ MinGW не в PATH
echo %PATH% | findstr /C:"MSYS2" >nul && echo ✅ MSYS2 в PATH || echo ❌ MSYS2 не в PATH
echo %PATH% | findstr /C:"PostgreSQL" >nul && echo ✅ PostgreSQL в PATH || echo ❌ PostgreSQL не в PATH

echo.
echo Проверка исполняемых файлов:
where cmake >nul && echo ✅ CMake найден || echo ❌ CMake не найден
where g++ >nul && echo ✅ G++ найден || echo ❌ G++ не найден
where gcc >nul && echo ✅ GCC найден || echo ❌ GCC не найден

echo.
pause