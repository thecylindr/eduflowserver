cmake_minimum_required(VERSION 3.15)
project(StudentManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Быстрая настройка компилятора
if(MSVC)
    add_compile_options(/W4 /EHsc)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Статическая линковка для MinGW
    if(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
        message(STATUS "Using static linking for MinGW")
    endif()
endif()

# Определение платформы
if(WIN32)
    add_definitions(-D_WINDOWS)
    set(PLATFORM_LIBS ws2_32 wsock32 crypt32)
    message(STATUS "Building for Windows")
else()
    add_definitions(-D_LINUX)
    find_package(Threads REQUIRED)
    set(PLATFORM_LIBS Threads::Threads)
    message(STATUS "Building for Linux")
endif()

# ПОИСК OPENSSL
message(STATUS "Looking for OpenSSL...")
if(WIN32)
    # Используем MSYS2 OpenSSL
    set(OPENSSL_PATHS "C:/msys64/mingw64")
    
    find_path(OPENSSL_INCLUDE_DIR
        NAMES openssl/evp.h
        PATHS ${OPENSSL_PATHS}
        PATH_SUFFIXES include
    )
    
    find_library(OPENSSL_CRYPTO_LIBRARY
        NAMES libcrypto-3-x64 libcrypto
        PATHS ${OPENSSL_PATHS}
        PATH_SUFFIXES lib
    )
    
    find_library(OPENSSL_SSL_LIBRARY
        NAMES libssl-3-x64 libssl
        PATHS ${OPENSSL_PATHS}
        PATH_SUFFIXES lib
    )
    
else()
    # Linux: используем стандартный поиск
    find_package(OpenSSL REQUIRED)
    if(OpenSSL_FOUND)
        set(OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR})
        set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_CRYPTO_LIBRARIES})
        set(OPENSSL_SSL_LIBRARY ${OPENSSL_SSL_LIBRARIES})
        set(OPENSSL_FOUND TRUE)
        message(STATUS "OpenSSL found via find_package")
    else()
        # Резервный поиск для Linux
        find_path(OPENSSL_INCLUDE_DIR
            NAMES openssl/evp.h
            PATHS
                /usr/include
                /usr/local/include
                /opt/openssl/include
        )
        
        find_library(OPENSSL_CRYPTO_LIBRARY
            NAMES crypto libcrypto
            PATHS
                /usr/lib
                /usr/lib64
                /usr/local/lib
                /opt/openssl/lib
        )
        
        find_library(OPENSSL_SSL_LIBRARY
            NAMES ssl libssl
            PATHS
                /usr/lib
                /usr/lib64
                /usr/local/lib
                /opt/openssl/lib
        )
    endif()
endif()

# Проверка OpenSSL
if(OPENSSL_INCLUDE_DIR AND OPENSSL_CRYPTO_LIBRARY AND OPENSSL_SSL_LIBRARY)
    set(OPENSSL_FOUND TRUE)
    message(STATUS "OpenSSL found:")
    message(STATUS "  Include: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "  Crypto: ${OPENSSL_CRYPTO_LIBRARY}")
    message(STATUS "  SSL: ${OPENSSL_SSL_LIBRARY}")
    
    # Получаем версию OpenSSL
    if(OpenSSL_VERSION)
        set(OPENSSL_VERSION "${OpenSSL_VERSION}")
    else()
        set(OPENSSL_VERSION "Unknown")
    endif()
    message(STATUS "  Version: ${OPENSSL_VERSION}")
    
    # Определяем путь к bin для DLL (только для Windows)
    if(WIN32)
        get_filename_component(OPENSSL_LIB_DIR "${OPENSSL_SSL_LIBRARY}" DIRECTORY)
        set(OPENSSL_BIN_DIR "${OPENSSL_LIB_DIR}/../bin")
        message(STATUS "  OpenSSL bin: ${OPENSSL_BIN_DIR}")
    endif()
else()
    set(OPENSSL_FOUND FALSE)
    message(WARNING "OpenSSL not found - hash functions will not work!")
endif()

# БЫСТРЫЙ ПОИСК POSTGRESQL
if(WIN32)
    # Используем установленный PostgreSQL
    set(QUICK_PG_PATHS
        "C:/Program Files/PostgreSQL/17"
        "C:/Program Files/PostgreSQL/16" 
    )
    
    foreach(QUICK_PATH IN LISTS QUICK_PG_PATHS)
        if(EXISTS "${QUICK_PATH}/include/libpq-fe.h")
            set(POSTGRESQL_INCLUDE_DIR "${QUICK_PATH}/include")
            set(POSTGRESQL_LIBRARY "${QUICK_PATH}/lib/libpq.lib")
            set(POSTGRESQL_BIN_DIR "${QUICK_PATH}/bin")
            message(STATUS "Found PostgreSQL at: ${QUICK_PATH}")
            break()
        endif()
    endforeach()
    
else()
    # Linux: используем pkg-config или стандартный поиск
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBPQ QUIET libpq)
        if(LIBPQ_FOUND)
            set(POSTGRESQL_INCLUDE_DIR ${LIBPQ_INCLUDE_DIRS})
            set(POSTGRESQL_LIBRARY ${LIBPQ_LIBRARIES})
            message(STATUS "Found PostgreSQL via pkg-config")
        endif()
    endif()
    
    # Резервный поиск для Linux
    if(NOT POSTGRESQL_INCLUDE_DIR)
        find_path(POSTGRESQL_INCLUDE_DIR 
            NAMES libpq-fe.h
            PATHS
                /usr/include/postgresql
                /usr/include
                /usr/local/include
                /usr/include/postgresql/*/server
        )
    endif()
    
    if(NOT POSTGRESQL_LIBRARY)
        find_library(POSTGRESQL_LIBRARY 
            NAMES pq
            PATHS
                /usr/lib
                /usr/local/lib
                /usr/lib/x86_64-linux-gnu
        )
    endif()
endif()

# Проверка найденных путей
if(NOT POSTGRESQL_INCLUDE_DIR)
    message(FATAL_ERROR "PostgreSQL include directory not found! Install libpq-dev or postgresql-devel")
endif()

if(NOT POSTGRESQL_LIBRARY)
    message(FATAL_ERROR "PostgreSQL library not found! Install libpq-dev or postgresql-devel")
endif()

# Для Windows проверяем bin directory
if(WIN32 AND NOT POSTGRESQL_BIN_DIR)
    message(FATAL_ERROR "PostgreSQL bin directory not found!")
endif()

message(STATUS "PostgreSQL include: ${POSTGRESQL_INCLUDE_DIR}")
message(STATUS "PostgreSQL library: ${POSTGRESQL_LIBRARY}")
if(WIN32)
    message(STATUS "PostgreSQL bin: ${POSTGRESQL_BIN_DIR}")
endif()

# ПРОВЕРКА СУЩЕСТВОВАНИЯ ИСХОДНЫХ ФАЙЛОВ
message(STATUS "Checking source files...")
set(SOURCES)
set(HEADERS)

# Проверяем каждый файл индивидуально
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    list(APPEND SOURCES "src/main.cpp")
    message(STATUS "Found: src/main.cpp")
else()
    message(FATAL_ERROR "Missing required file: src/main.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/database/DatabaseSetup.cpp")
    list(APPEND SOURCES "database/DatabaseSetup.cpp")
    message(STATUS "Found: database/DatabaseSetup.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/database/DatabaseCrud.cpp")
    list(APPEND SOURCES "database/DatabaseCrud.cpp")
    message(STATUS "Found: database/DatabaseCrud.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/database/DatabaseSessions.cpp")
    list(APPEND SOURCES "database/DatabaseSessions.cpp")
    message(STATUS "Found: database/DatabaseSessions.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configs/ConfigManager.cpp")
    list(APPEND SOURCES "configs/ConfigManager.cpp")
    message(STATUS "Found: configs/ConfigManager.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api/ApiServiceMain.cpp")
    list(APPEND SOURCES "api/ApiServiceMain.cpp")
    message(STATUS "Found: api/ApiServiceMain.cpp")
else()
    message(FATAL_ERROR "Missing required file: api/ApiServiceMain.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api/ApiServiceProfile.cpp")
    list(APPEND SOURCES "api/ApiServiceProfile.cpp")
    message(STATUS "Found: api/ApiServiceProfile.cpp")
else()
    message(FATAL_ERROR "Missing required file: api/ApiServiceProfile.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api/ApiServiceGetters.cpp")
    list(APPEND SOURCES "api/ApiServiceGetters.cpp")
    message(STATUS "Found: api/ApiServiceGetters.cpp")
else()
    message(FATAL_ERROR "Missing required file: api/ApiServiceGetters.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api/ApiServiceEvents.cpp")
    list(APPEND SOURCES "api/ApiServiceEvents.cpp")
    message(STATUS "Found: api/ApiServiceEvents.cpp")
else()
    message(FATAL_ERROR "Missing required file: api/ApiServiceEvents.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api/ApiServiceSessions.cpp")
    list(APPEND SOURCES "api/ApiServiceSessions.cpp")
    message(STATUS "Found: api/ApiServiceSessions.cpp")
else()
    message(FATAL_ERROR "Missing required file: api/ApiServiceSessions.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api/ApiServiceCrud.cpp")
    list(APPEND SOURCES "api/ApiServiceCrud.cpp")
    message(STATUS "Found: api/ApiServiceCrud.cpp")
else()
    message(FATAL_ERROR "Missing required file: api/ApiServiceCrud.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api/ApiServiceAuth.cpp")
    list(APPEND SOURCES "api/ApiServiceAuth.cpp")
    message(STATUS "Found: api/ApiServiceAuth.cpp")
else()
    message(FATAL_ERROR "Missing required file: api/ApiServiceAuth.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/LocaleManager.cpp")
    list(APPEND SOURCES "src/LocaleManager.cpp")
    message(STATUS "Found: src/LocaleManager.cpp")
endif()

# Проверяем заголовочные файлы
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/database/DatabaseService.h")
    list(APPEND HEADERS "database/DatabaseService.h")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configs/ConfigManager.h")
    list(APPEND HEADERS "configs/ConfigManager.h")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api/ApiService.h")
    list(APPEND HEADERS "api/ApiService.h")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/LocaleManager.h")
    list(APPEND HEADERS "src/LocaleManager.h")
endif()

# Проверяем наличие json.hpp
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/json.hpp")
    message(STATUS "Found local json.hpp - using bundled JSON library")
    list(APPEND HEADERS "src/json.hpp")
else()
    message(FATAL_ERROR "JSON library not found! Please place json.hpp in src/ directory")
endif()

if(NOT SOURCES)
    message(FATAL_ERROR "No source files found in src/ directory!")
endif()

message(STATUS "Total source files: ${SOURCES}")

# Создание исполняемого файла
add_executable(StudentManagementSystem ${SOURCES} ${HEADERS})

# Подключение библиотек
target_include_directories(StudentManagementSystem PRIVATE 
    ${POSTGRESQL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Добавляем OpenSSL если найден
if(OPENSSL_FOUND)
    target_include_directories(StudentManagementSystem PRIVATE ${OPENSSL_INCLUDE_DIR})
endif()

target_link_libraries(StudentManagementSystem PRIVATE
    ${POSTGRESQL_LIBRARY}
    ${PLATFORM_LIBS}
)

if(OPENSSL_FOUND)
    target_link_libraries(StudentManagementSystem PRIVATE
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
    )
    message(STATUS "Linking with OpenSSL libraries")
else()
    message(WARNING "Building without OpenSSL - hash functions will not work!")
endif()

# Копирование конфига
if(EXISTS "${CMAKE_SOURCE_DIR}/config.json")
    configure_file("${CMAKE_SOURCE_DIR}/config.json" "${CMAKE_BINARY_DIR}/config.json" COPYONLY)
    message(STATUS "Config file copied")
else()
    message(WARNING "config.json not found - creating default config")
    file(WRITE "${CMAKE_BINARY_DIR}/config.json" "{\n    \"database\": {\n        \"host\": \"localhost\",\n        \"port\": 5432,\n        \"dbname\": \"student_management\",\n        \"user\": \"postgres\",\n        \"password\": \"password\"\n    },\n    \"api\": {\n        \"port\": 8080,\n        \"host\": \"localhost\"\n    },\n    \"locale\": \"en_US\"\n}")
    message(STATUS "Default config.json created in build directory")
endif()

# КОПИРОВАНИЕ DLL ТОЛЬКО ДЛЯ WINDOWS
if(WIN32)
    message(STATUS "Setting up DLL copy for Windows...")
    
    # Создаем цель для копирования только проверенных DLL
    add_custom_target(copy_required_dlls ALL
        COMMENT "Copying required DLL files..."
    )
    
    # Функция для копирования только существующих DLL
    function(copy_required_dll source_dir dll_name)
        if(EXISTS "${source_dir}/${dll_name}")
            add_custom_command(TARGET copy_required_dlls POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${source_dir}/${dll_name}"
                    "${CMAKE_BINARY_DIR}"
                COMMENT "Copying ${dll_name}"
            )
            message(STATUS "Will copy: ${dll_name}")
        else()
            message(STATUS "Skipping (not found): ${dll_name}")
        endif()
    endfunction()

    # Копируем только необходимые DLL из PostgreSQL
    copy_required_dll("${POSTGRESQL_BIN_DIR}" "libpq.dll")
    copy_required_dll("${POSTGRESQL_BIN_DIR}" "libintl-9.dll")
    
    # Копируем OpenSSL и системные DLL из MSYS2
    if(MINGW)
        set(MINGW_BIN_DIR "C:/msys64/mingw64/bin")
        if(EXISTS "${MINGW_BIN_DIR}")
            copy_required_dll("${MINGW_BIN_DIR}" "libssl-3-x64.dll")
            copy_required_dll("${MINGW_BIN_DIR}" "libcrypto-3-x64.dll")
            copy_required_dll("${MINGW_BIN_DIR}" "libiconv-2.dll")
            copy_required_dll("${MINGW_BIN_DIR}" "libwinpthread-1.dll")
        endif()
    endif()
    
    # Проверка результата
    add_custom_command(TARGET copy_required_dlls POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Required DLL copy completed"
    )
else()
    message(STATUS "Skipping DLL copy (not on Windows)")
endif()

# Настройка выходных директорий
set_target_properties(StudentManagementSystem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Финальное сообщение
message(STATUS "")
message(STATUS "=== Configuration Summary ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Source files: ${SOURCES}")
message(STATUS "PostgreSQL: ${POSTGRESQL_INCLUDE_DIR}")
if(OPENSSL_FOUND)
    message(STATUS "OpenSSL: Found (${OPENSSL_VERSION})")
else()
    message(STATUS "OpenSSL: NOT FOUND - hash functions disabled")
endif()
if(WIN32)
    message(STATUS "Output: ${CMAKE_BINARY_DIR}/StudentManagementSystem.exe")
    message(STATUS "DLL strategy: Copying required DLLs")
else()
    message(STATUS "Output: ${CMAKE_BINARY_DIR}/StudentManagementSystem")
endif()
message(STATUS "==============================")
