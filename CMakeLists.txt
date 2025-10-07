cmake_minimum_required(VERSION 3.15)
project(StudentManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Детектим ОС
if(WIN32)
    add_compile_definitions(_WINDOWS)
    set(PLATFORM_LIBS ws2_32 wsock32)
    message(STATUS "Building for Windows")
else()
    add_compile_definitions(_LINUX)
    set(PLATFORM_LIBS pthread)
    message(STATUS "Building for Linux")
endif()

# Поиск PostgreSQL
if(WIN32)
    # Windows: ищем PostgreSQL 17 в стандартных путях
    set(PG_PATHS
        "C:/Program Files/PostgreSQL/17"
        "$ENV{PROGRAMFILES}/PostgreSQL/17"
        "C:/PostgreSQL/17"
    )
    
    find_path(POSTGRESQL_INCLUDE_DIR
        NAMES libpq-fe.h
        PATHS ${PG_PATHS}
        PATH_SUFFIXES include
    )
    
    find_library(POSTGRESQL_LIBRARY
        NAMES libpq libpq64
        PATHS ${PG_PATHS}
        PATH_SUFFIXES lib
    )
    
else()
    # Linux: используем pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBPQ REQUIRED libpq)
    
    set(POSTGRESQL_INCLUDE_DIR ${LIBPQ_INCLUDE_DIRS})
    set(POSTGRESQL_LIBRARY ${LIBPQ_LIBRARIES})
endif()

# Проверка найденных путей
if(NOT POSTGRESQL_INCLUDE_DIR)
    if(WIN32)
        message(FATAL_ERROR "
        PostgreSQL 17 include directory not found!
        Проверьте пути:
        - C:/Program Files/PostgreSQL/17/include
        - Установлен ли PostgreSQL 17?
        - Правильно ли указана версия?
        ")
    else()
        message(FATAL_ERROR "
        PostgreSQL development libraries not found!
        Установите: sudo apt install libpq-dev
        ")
    endif()
endif()

if(NOT POSTGRESQL_LIBRARY)
    if(WIN32)
        message(FATAL_ERROR "
        PostgreSQL 17 library not found!
        Проверьте пути:
        - C:/Program Files/PostgreSQL/17/lib/libpq.lib
        ")
    else()
        message(FATAL_ERROR "
        PostgreSQL library not found!
        Установите: sudo apt install libpq-dev
        ")
    endif()
endif()

message(STATUS "PostgreSQL include: ${POSTGRESQL_INCLUDE_DIR}")
message(STATUS "PostgreSQL library: ${POSTGRESQL_LIBRARY}")

# Исходные файлы
add_executable(StudentManagementSystem
    src/main.cpp
    src/DatabaseService.cpp
    src/ConfigManager.cpp
    src/ApiService.cpp
    src/LocaleManager.cpp
)

# Подключение библиотек
target_include_directories(StudentManagementSystem PRIVATE 
    ${POSTGRESQL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(StudentManagementSystem 
    ${POSTGRESQL_LIBRARY}
    ${PLATFORM_LIBS}
)

# Копирование DLL для Windows
if(WIN32)
    add_custom_command(TARGET StudentManagementSystem POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${POSTGRESQL_LIBRARY_DIR}/../bin/libpq.dll"
            $<TARGET_FILE_DIR:StudentManagementSystem>
        COMMENT "Copying PostgreSQL DLL"
    )
endif()

set_target_properties(StudentManagementSystem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)