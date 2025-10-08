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
    set(PLATFORM_LIBS ws2_32 wsock32)
    message(STATUS "Building for Windows")
else()
    add_definitions(-D_LINUX)
    find_package(Threads REQUIRED)
    set(PLATFORM_LIBS Threads::Threads)
    message(STATUS "Building for Linux")
endif()

# БЫСТРЫЙ ПОИСК POSTGRESQL
if(WIN32)
    # Сначала проверяем установленный PostgreSQL
    set(QUICK_PG_PATHS
        "C:/Program Files/PostgreSQL/17"
        "C:/Program Files/PostgreSQL/16" 
        "$ENV{PROGRAMFILES}/PostgreSQL/17"
        "$ENV{PROGRAMFILES}/PostgreSQL/16"
    )
    
    # Быстрый поиск в основных путях
    foreach(QUICK_PATH IN LISTS QUICK_PG_PATHS)
        if(EXISTS "${QUICK_PATH}/include/libpq-fe.h")
            set(POSTGRESQL_INCLUDE_DIR "${QUICK_PATH}/include")
            set(POSTGRESQL_LIBRARY "${QUICK_PATH}/lib/libpq.lib")
            set(POSTGRESQL_BIN_DIR "${QUICK_PATH}/bin")
            message(STATUS "Quick found PostgreSQL at: ${QUICK_PATH}")
            break()
        endif()
    endforeach()
    
    # Если не нашли, используем MSYS2 как запасной вариант
    if(NOT POSTGRESQL_INCLUDE_DIR)
        if(EXISTS "C:/msys64/mingw64/include/libpq-fe.h")
            set(POSTGRESQL_INCLUDE_DIR "C:/msys64/mingw64/include")
            set(POSTGRESQL_LIBRARY "C:/msys64/mingw64/lib/libpq.dll.a")
            set(POSTGRESQL_BIN_DIR "C:/msys64/mingw64/bin")
            message(STATUS "Using PostgreSQL from MSYS2")
        endif()
    endif()
    
else()
    # Linux: используем pkg-config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBPQ QUIET libpq)
        if(LIBPQ_FOUND)
            set(POSTGRESQL_INCLUDE_DIR ${LIBPQ_INCLUDE_DIRS})
            set(POSTGRESQL_LIBRARY ${LIBPQ_LIBRARIES})
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
        )
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
    message(FATAL_ERROR "PostgreSQL include directory not found!")
endif()

if(NOT POSTGRESQL_LIBRARY)
    message(FATAL_ERROR "PostgreSQL library not found!")
endif()

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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/DatabaseService.cpp")
    list(APPEND SOURCES "src/DatabaseService.cpp")
    message(STATUS "Found: src/DatabaseService.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigManager.cpp")
    list(APPEND SOURCES "src/ConfigManager.cpp")
    message(STATUS "Found: src/ConfigManager.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ApiService.cpp")
    list(APPEND SOURCES "src/ApiService.cpp")
    message(STATUS "Found: src/ApiService.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/LocaleManager.cpp")
    list(APPEND SOURCES "src/LocaleManager.cpp")
    message(STATUS "Found: src/LocaleManager.cpp")
endif()

# Проверяем заголовочные файлы
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/DatabaseService.h")
    list(APPEND HEADERS "src/DatabaseService.h")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigManager.h")
    list(APPEND HEADERS "src/ConfigManager.h")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ApiService.h")
    list(APPEND HEADERS "src/ApiService.h")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/LocaleManager.h")
    list(APPEND HEADERS "src/LocaleManager.h")
endif()

# Проверяем наличие json.hpp
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/json.hpp")
    message(STATUS "Found local json.hpp - using bundled JSON library")
    list(APPEND HEADERS "src/json.hpp")
else()
    message(WARNING "json.hpp not found in src/ directory")
    # Если json.hpp нет, можно попробовать найти системную установку
    find_path(JSON_INCLUDE_DIR
        NAMES nlohmann/json.hpp
        PATHS
            /usr/include
            /usr/local/include
    )
    if(JSON_INCLUDE_DIR)
        message(STATUS "Found system JSON library: ${JSON_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "JSON library not found! Please place json.hpp in src/ directory")
    endif()
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
)

target_link_libraries(StudentManagementSystem 
    ${POSTGRESQL_LIBRARY}
    ${PLATFORM_LIBS}
)

# Копирование конфига
configure_file("${CMAKE_SOURCE_DIR}/config.json" "${CMAKE_BINARY_DIR}/config.json" COPYONLY)

# АВТОМАТИЧЕСКОЕ КОПИРОВАНИЕ DLL ДЛЯ WINDOWS
if(WIN32)
    message(STATUS "Setting up DLL copy for Windows...")
    
    # Создаем цель для копирования DLL
    add_custom_target(copy_dlls ALL
        COMMENT "Copying PostgreSQL DLL files..."
        DEPENDS StudentManagementSystem
    )
    
    # Функция для копирования DLL если существует
    function(copy_dll_if_exists dll_name)
        if(EXISTS "${POSTGRESQL_BIN_DIR}/${dll_name}")
            add_custom_command(TARGET copy_dlls POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${POSTGRESQL_BIN_DIR}/${dll_name}"
                    "$<TARGET_FILE_DIR:StudentManagementSystem>"
                COMMENT "Copying ${dll_name}"
            )
            message(STATUS "Will copy: ${dll_name}")
        else()
            message(STATUS "DLL not found (optional): ${dll_name}")
        endif()
    endfunction()

    # Копируем основные DLL
    copy_dll_if_exists("libpq.dll")
    copy_dll_if_exists("libintl-8.dll")
    copy_dll_if_exists("libintl-9.dll")
    copy_dll_if_exists("libcrypto-3-x64.dll")
    copy_dll_if_exists("libssl-3-x64.dll")
    copy_dll_if_exists("libcrypto-1_1-x64.dll")
    copy_dll_if_exists("libssl-1_1-x64.dll")
    
    # Проверка результата
    add_custom_command(TARGET copy_dlls POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "DLL copy completed"
    )
endif()

# Настройка выходных директорий
set_target_properties(StudentManagementSystem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ГАРАНТИРОВАННОЕ КОПИРОВАНИЕ ВСЕХ DLL
if(WIN32)
    # Создаем отдельную цель для копирования ВСЕХ DLL
    add_custom_target(copy_all_dlls ALL
        COMMENT "Copying ALL required DLL files..."
    )
    
    # Функция для копирования всех DLL по шаблону
    function(copy_all_dlls_from_dir dir_path pattern)
        if(EXISTS "${dir_path}")
            file(GLOB dll_files "${dir_path}/${pattern}")
            foreach(dll_file IN LISTS dll_files)
                get_filename_component(dll_name "${dll_file}" NAME)
                add_custom_command(TARGET copy_all_dlls POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll_file}"
                        "${CMAKE_BINARY_DIR}"
                    COMMENT "Copying ${dll_name}"
                )
            endforeach()
        endif()
    endfunction()

    # Копируем все DLL из PostgreSQL
    copy_all_dlls_from_dir("${POSTGRESQL_BIN_DIR}" "lib*.dll")
    
    # Копируем все DLL из MinGW (если используется MinGW)
    if(MINGW)
        get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        copy_all_dlls_from_dir("${MINGW_BIN_DIR}" "lib*.dll")
        copy_all_dlls_from_dir("C:/msys64/mingw64/bin" "lib*.dll")
    endif()
    
    # Сообщение о завершении
    add_custom_command(TARGET copy_all_dlls POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "All DLLs copied to ${CMAKE_BINARY_DIR}"
    )
endif()

# Финальное сообщение
message(STATUS "")
message(STATUS "=== Configuration Summary ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Source files: ${SOURCES}")
message(STATUS "PostgreSQL: ${POSTGRESQL_INCLUDE_DIR}")
if(WIN32)
    message(STATUS "Output: ${CMAKE_BINARY_DIR}/StudentManagementSystem.exe")
else()
    message(STATUS "Output: ${CMAKE_BINARY_DIR}/StudentManagementSystem")
endif()
message(STATUS "==============================")