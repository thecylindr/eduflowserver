@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ========================================
echo    📥 Auto Download Resources - Student Management
echo ========================================
echo.

:: Configuration
set "ROOT_DIR=%~dp0"
set "BUILD_TOOLS=%ROOT_DIR%build_tools"
set "CACHE_DIR=%ROOT_DIR%cache"
set "LOGS_DIR=%ROOT_DIR%logs"

:: Create directories
for %%d in ("!BUILD_TOOLS!" "!CACHE_DIR!" "!LOGS_DIR!") do (
    if not exist "%%d" mkdir "%%d" >nul 2>&1
)

:: Function to download and extract files
:download_and_extract
set "URL=%~1"
set "OUTPUT=%~2"
set "EXTRACT_DIR=%~3"
set "CHECK_FILE=%~4"

echo.
echo 📥 Downloading: !OUTPUT!

if not exist "!CACHE_DIR!\!OUTPUT!" (
    powershell -Command "Invoke-WebRequest -Uri '!URL!' -OutFile '!CACHE_DIR!\!OUTPUT!'" >nul 2>&1
    if !errorlevel! neq 0 (
        echo ❌ Download failed: !OUTPUT!
        exit /b 1
    )
)

if not "!EXTRACT_DIR!"=="" (
    if not exist "!BUILD_TOOLS!\!EXTRACT_DIR!\!CHECK_FILE!" (
        echo 🗜️ Extracting: !OUTPUT!
        if "!OUTPUT:~-4!"==".zip" (
            powershell -Command "Expand-Archive -Path '!CACHE_DIR!\!OUTPUT!' -DestinationPath '!BUILD_TOOLS!\!EXTRACT_DIR!' -Force" >nul 2>&1
        ) else if "!OUTPUT:~-7!"==".tar.gz" (
            cd !BUILD_TOOLS!
            tar -xzf "!CACHE_DIR!\!OUTPUT!" >nul 2>&1
        )
        
        if exist "!BUILD_TOOLS!\!EXTRACT_DIR!\!CHECK_FILE!" (
            echo ✅ Extracted: !EXTRACT_DIR!
        ) else (
            echo ❌ Extraction failed: !EXTRACT_DIR!
            exit /b 1
        )
    ) else (
        echo ✅ Already exists: !EXTRACT_DIR!
    )
) else (
    echo ✅ Downloaded: !OUTPUT!
)

exit /b 0

:: PostgreSQL Installation
echo ========================================
echo    🗃️ POSTGRESQL SETUP
echo ========================================

set "PG_VERSION=17.2-1"
set "PG_DIR=!BUILD_TOOLS!\pgsql"

if not exist "!PG_DIR!\bin\psql.exe" (
    echo.
    echo 📥 Installing PostgreSQL...
    
    call :download_and_extract "https://get.enterprisedb.com/postgresql/postgresql-!PG_VERSION!-windows-x64-binaries.zip" "postgresql-!PG_VERSION!-windows.zip" "pgsql" "bin\psql.exe"
    
    if !errorlevel! neq 0 (
        echo ❌ PostgreSQL installation failed
        pause
        exit /b 1
    )
    
    :: Initialize database
    echo 🗃️ Initializing database...
    if not exist "!PG_DIR!\data" mkdir "!PG_DIR!\data"
    "!PG_DIR!\bin\initdb.exe" -D "!PG_DIR!\data" -U postgres --encoding=UTF8 --no-locale --auth=trust >nul 2>&1
    
    :: Create startup script
    (
        echo @echo off
        echo chcp 65001 ^>nul
        echo echo Starting PostgreSQL...
        echo "!PG_DIR!\bin\pg_ctl.exe" -D "!PG_DIR!\data" -l "!LOGS_DIR!\postgres.log" start
        echo timeout /t 3 ^>nul
        echo echo PostgreSQL started on port 5432
    ) > "!BUILD_TOOLS!\start_postgres.bat"
    
    (
        echo @echo off
        echo chcp 65001 ^>nul
        echo echo Stopping PostgreSQL...
        echo "!PG_DIR!\bin\pg_ctl.exe" -D "!PG_DIR!\data" stop
        echo echo PostgreSQL stopped
    ) > "!BUILD_TOOLS!\stop_postgres.bat"
    
    echo ✅ PostgreSQL ready in: !PG_DIR!
) else (
    echo ✅ PostgreSQL already installed
)

:: Compiler Installation (MinGW-w64)
echo.
echo ========================================
echo    🔧 COMPILER SETUP
echo ========================================

set "MINGW_DIR=!BUILD_TOOLS!\mingw64"

if not exist "!MINGW_DIR!\bin\g++.exe" (
    echo.
    echo 📥 Installing MinGW-w64 compiler...
    
    call :download_and_extract "https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0-16.0.6-11.0.0-msvcrt-r2/winlibs-x86_64-posix-seh-gcc-13.2.0-llvm-16.0.6-mingw-w64msvcrt-11.0.0-r2.zip" "mingw-w64.zip" "mingw64" "bin\g++.exe"
    
    if !errorlevel! neq 0 (
        echo ❌ Compiler installation failed
        pause
        exit /b 1
    )
    
    echo ✅ Compiler ready in: !MINGW_DIR!
) else (
    echo ✅ Compiler already installed
)

:: Build Tools (CMake, Ninja)
echo.
echo ========================================
echo    🛠️ BUILD TOOLS SETUP
echo ========================================

:: CMake
if not exist "!BUILD_TOOLS!\cmake\bin\cmake.exe" (
    echo.
    echo 📥 Installing CMake...
    
    call :download_and_extract "https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-windows-x86_64.zip" "cmake-3.28.1.zip" "cmake" "bin\cmake.exe"
    
    if !errorlevel! neq 0 (
        echo ❌ CMake installation failed
    ) else (
        echo ✅ CMake ready
    )
) else (
    echo ✅ CMake already installed
)

:: Ninja
if not exist "!BUILD_TOOLS!\ninja\ninja.exe" (
    echo.
    echo 📥 Installing Ninja...
    
    call :download_and_extract "https://github.com/ninja-build/ninja/releases/latest/download/ninja-win.zip" "ninja-win.zip" "ninja" "ninja.exe"
    
    if !errorlevel! neq 0 (
        echo ❌ Ninja installation failed
    ) else (
        echo ✅ Ninja ready
    )
) else (
    echo ✅ Ninja already installed
)

:: Update PATH environment
echo.
echo ========================================
echo    ⚙️ ENVIRONMENT SETUP
echo ========================================

set "PATH=!MINGW_DIR!\bin;!BUILD_TOOLS!\cmake\bin;!BUILD_TOOLS!\ninja;!PG_DIR!\bin;!PATH!"

:: Verify installations
echo.
echo 🔍 Verifying installations...

echo - Checking PostgreSQL...
!PG_DIR!\bin\psql.exe --version >nul 2>&1
if !errorlevel! equ 0 (
    for /f "tokens=*" %%i in ('!PG_DIR!\bin\psql.exe --version 2^>^&1') do echo ✅ %%i
) else (
    echo ❌ PostgreSQL check failed
)

echo - Checking C++ compiler...
!MINGW_DIR!\bin\g++.exe --version >nul 2>&1
if !errorlevel! equ 0 (
    for /f "tokens=*" %%i in ('!MINGW_DIR!\bin\g++.exe --version 2^>^&1 ^| findstr "g++"') do echo ✅ %%i
) else (
    echo ❌ Compiler check failed
)

echo - Checking CMake...
if exist "!BUILD_TOOLS!\cmake\bin\cmake.exe" (
    for /f "tokens=*" %%i in ('!BUILD_TOOLS!\cmake\bin\cmake.exe --version 2^>^&1') do echo ✅ %%i
) else (
    echo ❌ CMake check failed
)

echo - Checking Ninja...
if exist "!BUILD_TOOLS!\ninja\ninja.exe" (
    !BUILD_TOOLS!\ninja\ninja.exe --version >nul 2>&1
    if !errorlevel! equ 0 (
        for /f "tokens=*" %%i in ('!BUILD_TOOLS!\ninja\ninja.exe --version 2^>^&1') do echo ✅ Ninja %%i
    )
) else (
    echo ❌ Ninja check failed
)

:: Create environment script
echo.
echo 📄 Creating environment script...
(
    echo @echo off
    echo chcp 65001 ^>nul
    echo.
    echo echo ========================================
    echo echo    🚀 Student Management - Development Environment
    echo echo ========================================
    echo echo.
    echo.
    echo setlocal enabledelayedexpansion
    echo.
    echo :: Set paths
    echo set "ROOT_DIR=%~dp0"
    echo set "BUILD_TOOLS=!ROOT_DIR!build_tools"
    echo set "PG_DIR=!BUILD_TOOLS!\pgsql"
    echo set "MINGW_DIR=!BUILD_TOOLS!\mingw64"
    echo.
    echo :: Update PATH
    echo set "PATH=!MINGW_DIR!\bin;!BUILD_TOOLS!\cmake\bin;!BUILD_TOOLS!\ninja;!PG_DIR!\bin;!PATH!"
    echo.
    echo echo ✅ Environment loaded
    echo echo.
    echo echo Available commands:
    echo echo - start_postgres.bat : Start PostgreSQL database
    echo echo - stop_postgres.bat  : Stop PostgreSQL database
    echo echo - g++                : C++ compiler
    echo echo - cmake              : Build system
    echo echo - ninja              : Build tool
    echo echo - psql               : PostgreSQL client
    echo.
    echo cmd /k
) > "dev_env.bat"

:: Final report
echo.
echo ========================================
echo    ✅ RESOURCE DOWNLOAD COMPLETED!
echo ========================================
echo.
echo 📊 Installed components:
echo    ✅ PostgreSQL 17 (portable)
echo    ✅ MinGW-w64 GCC compiler
echo    ✅ CMake 3.28.1
echo    ✅ Ninja build system
echo.
echo 📁 Installation directories:
echo    - build_tools\pgsql\     : PostgreSQL
echo    - build_tools\mingw64\   : Compiler
echo    - build_tools\cmake\     : CMake
echo    - build_tools\ninja\     : Ninja
echo    - cache\                 : Downloaded packages
echo    - logs\                  : Log files
echo.
echo 🚀 Next steps:
echo    1. Run 'dev_env.bat' to open development environment
echo    2. Run 'build_tools\start_postgres.bat' to start database
echo    3. Compile your project with build scripts
echo.
echo ⚠️  Note: PostgreSQL is configured with:
echo     - Username: postgres
echo     - Password: [none - trust authentication]
echo     - Port: 5432
echo     - Data: build_tools\pgsql\data
echo.

pause