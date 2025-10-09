#!/bin/bash

# PostgreSQL Project Builder
echo "========================================"
echo "   PostgreSQL Project Builder"
echo "========================================"
echo

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функции для красивого вывода
print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# Функция проверки команды
command_exists() {
    command -v "$1" &> /dev/null
}

# Функция определения дистрибутива
detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$NAME
        OS_ID=$ID
    else
        OS=$(uname -s)
        OS_ID=$(uname -s | tr '[:upper:]' '[:lower:]')
    fi
}

# Функция установки пакета
install_package() {
    local package=$1
    print_info "Установка: $package"
    
    case $OS_ID in
        ubuntu|debian|linuxmint)
            sudo apt update && sudo apt install -y "$package"
            ;;
        centos|rhel|fedora)
            if command_exists dnf; then
                sudo dnf install -y "$package"
            else
                sudo yum install -y "$package"
            fi
            ;;
        arch|manjaro)
            sudo pacman -S --noconfirm "$package"
            ;;
        opensuse*)
            sudo zypper install -y "$package"
            ;;
        *)
            print_error "Неизвестный дистрибутив: $OS"
            return 1
            ;;
    esac
}

# Функция установки CMake
install_cmake() {
    print_info "Установка CMake..."
    
    case $OS_ID in
        ubuntu|debian|linuxmint)
            install_package "cmake"
            ;;
        centos|rhel)
            if command_exists dnf; then
                sudo dnf install -y epel-release
                sudo dnf install -y cmake
            else
                sudo yum install -y epel-release
                sudo yum install -y cmake
            fi
            ;;
        fedora)
            sudo dnf install -y cmake
            ;;
        arch|manjaro)
            install_package "cmake"
            ;;
        opensuse*)
            install_package "cmake"
            ;;
        *)
            print_error "Ручная установка CMake не реализована для $OS"
            return 1
            ;;
    esac
}

# Функция установки PostgreSQL development libraries
install_postgresql_dev() {
    print_info "Установка PostgreSQL development libraries..."
    
    case $OS_ID in
        ubuntu|debian|linuxmint)
            install_package "libpq-dev postgresql-server-dev-all"
            ;;
        centos|rhel|fedora)
            if command_exists dnf; then
                sudo dnf install -y postgresql-devel
            else
                sudo yum install -y postgresql-devel
            fi
            ;;
        arch|manjaro)
            install_package "postgresql-libs"
            ;;
        opensuse*)
            install_package "postgresql-devel"
            ;;
        *)
            print_error "Ручная установка PostgreSQL dev не реализована для $OS"
            return 1
            ;;
    esac
}

# Функция установки pkg-config
install_pkg_config() {
    print_info "Установка pkg-config..."
    
    case $OS_ID in
        ubuntu|debian|linuxmint)
            install_package "pkg-config"
            ;;
        centos|rhel|fedora)
            install_package "pkgconfig"
            ;;
        arch|manjaro)
            install_package "pkg-config"
            ;;
        opensuse*)
            install_package "pkg-config"
            ;;
        *)
            print_error "Ручная установка pkg-config не реализована для $OS"
            return 1
            ;;
    esac
}

# Функция проверки и установки компилятора
install_compiler() {
    print_info "Проверка компилятора C++..."
    
    if ! command_exists g++; then
        print_warning "Компилятор C++ не найден. Установка..."
        case $OS_ID in
            ubuntu|debian|linuxmint)
                install_package "build-essential"
                ;;
            centos|rhel|fedora)
                if command_exists dnf; then
                    sudo dnf groupinstall -y "Development Tools"
                else
                    sudo yum groupinstall -y "Development Tools"
                fi
                ;;
            arch|manjaro)
                install_package "base-devel"
                ;;
            opensuse*)
                install_package "patterns-devel-base-devel_basis"
                ;;
        esac
    fi
    
    if command_exists g++; then
        print_success "Компилятор C++: $(g++ --version | head -n1)"
    else
        print_error "Не удалось установить компилятор C++"
        return 1
    fi
}

# Функция проверки libpq
check_libpq() {
    print_info "Проверка библиотеки libpq..."
    
    if pkg-config --exists libpq; then
        print_success "libpq найден: $(pkg-config --modversion libpq)"
        return 0
    else
        print_error "libpq не найден через pkg-config"
        return 1
    fi
}

# Функция проверки необходимых файлов проекта
check_project_files() {
    print_info "Проверка файлов проекта..."
    
    local missing_files=()
    
    if [ ! -f "CMakeLists.txt" ]; then
        missing_files+=("CMakeLists.txt")
    fi
    
    if [ ! -f "src/main.cpp" ]; then
        missing_files+=("src/main.cpp")
    fi
    
    if [ ${#missing_files[@]} -ne 0 ]; then
        print_error "Отсутствуют необходимые файлы:"
        for file in "${missing_files[@]}"; do
            echo "  - $file"
        done
        echo
        echo "Текущая структура директорий:"
        ls -la
        return 1
    fi
    
    print_success "Все необходимые файлы найдены:"
    echo "  - CMakeLists.txt"
    echo "  - src/main.cpp"
    return 0
}

# Функция сборки проекта
build_project() {
    print_info "Сборка проекта..."
    
    # Очищаем предыдущую сборку
    if [ -d "build" ]; then
        print_warning "Очистка предыдущей сборки..."
        rm -rf build
    fi
    
    # Создаем директорию сборки
    mkdir -p build
    cd build
    
    echo
    print_info "[1/3] Конфигурация CMake..."
    if cmake ..; then
        print_success "Конфигурация CMake успешна"
    else
        print_error "Ошибка конфигурации CMake"
        return 1
    fi
    
    echo
    print_info "[2/3] Компиляция проекта..."
    if cmake --build .; then
        print_success "Компиляция завершена успешно"
    else
        print_error "Ошибка компиляции"
        return 1
    fi
    
    echo
    print_info "[3/3] Поиск исполняемого файла..."
    
    # Ищем исполняемый файл
    local executable=""
    if [ -f "bin/StudentManagementSystem" ]; then
        executable="bin/StudentManagementSystem"
    elif [ -f "StudentManagementSystem" ]; then
        executable="StudentManagementSystem"
    else
        # Пытаемся найти любой исполняемый файл
        executable=$(find . -maxdepth 2 -type f -executable ! -name "*.so" ! -name "*.dll" | head -n1)
        if [ -z "$executable" ]; then
            print_error "Исполняемый файл не найден"
            echo "Доступные файлы в build/:"
            ls -la
            return 1
        fi
    fi
    
    chmod +x "$executable"
    print_success "Исполняемый файл: $(pwd)/$executable"
    return 0
}

# Функция запуска программы
run_program() {
    print_info "Запуск программы..."
    echo
    
    cd build
    
    local executable=""
    if [ -f "bin/StudentManagementSystem" ]; then
        executable="./bin/StudentManagementSystem"
    elif [ -f "StudentManagementSystem" ]; then
        executable="./StudentManagementSystem"
    else
        executable=$(find . -maxdepth 2 -type f -executable ! -name "*.so" ! -name "*.dll" | head -n1)
        if [ -z "$executable" ]; then
            print_error "Не удалось найти исполняемый файл для запуска"
            return 1
        fi
    fi
    
    print_info "Выполняется: $executable"
    echo "----------------------------------------"
    "$executable"
    local exit_code=$?
    echo "----------------------------------------"
    echo "Код завершения: $exit_code"
    return $exit_code
}

# Функция показа информации о версиях
show_versions() {
    echo
    print_info "Информация о версиях:"
    if command_exists cmake; then
        echo "  CMake: $(cmake --version | head -n1)"
    fi
    if command_exists g++; then
        echo "  GCC: $(g++ --version | head -n1)"
    fi
    if pkg-config --exists libpq; then
        echo "  libpq: $(pkg-config --modversion libpq)"
    fi
    echo "  Дистрибутив: $OS"
    echo "  Рабочая директория: $(pwd)"
}

# Основная функция
main() {
    # Определяем ОС
    detect_os
    print_info "Обнаружена система: $OS"
    
    # Проверяем CMake
    if ! command_exists cmake; then
        print_warning "CMake не найден в системе"
        install_cmake
        if ! command_exists cmake; then
            print_error "Не удалось установить CMake"
            exit 1
        fi
    else
        print_success "CMake уже установлен: $(cmake --version | head -n1)"
    fi
    
    # Устанавливаем компилятор если нужно
    install_compiler || exit 1
    
    # Устанавливаем pkg-config если нужно
    if ! command_exists pkg-config; then
        install_pkg_config || exit 1
    fi
    
    # Устанавливаем PostgreSQL development libraries
    if ! check_libpq; then
        install_postgresql_dev
        if ! check_libpq; then
            print_error "Не удалось установить libpq"
            echo
            print_info "Попробуйте установить вручную:"
            case $OS_ID in
                ubuntu|debian|linuxmint)
                    echo "  sudo apt install libpq-dev postgresql-server-dev-all"
                    ;;
                centos|rhel|fedora)
                    echo "  sudo dnf install postgresql-devel"
                    ;;
                arch|manjaro)
                    echo "  sudo pacman -S postgresql-libs"
                    ;;
            esac
            exit 1
        fi
    fi
    
    # Показываем версии
    show_versions
    
    # Проверяем необходимые файлы проекта
    if ! check_project_files; then
        print_error "Проект не готов к сборке. Убедитесь что есть:"
        echo "  - CMakeLists.txt"
        echo "  - src/main.cpp"
        exit 1
    fi
    
    # Собираем проект
    if build_project; then
        echo
        print_success "✅ СБОРКА УСПЕШНО ЗАВЕРШЕНА!"
        echo
        run_program
    else
        print_error "Сборка не удалась"
        exit 1
    fi
}

# Проверка прав
if [ "$EUID" -eq 0 ]; then
    print_warning "Скрипт запущен от root. Рекомендуется запускать от обычного пользователя."
    read -p "Продолжить? [y/N] " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Запуск основной функции
main