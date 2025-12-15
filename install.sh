#!/bin/bash

# PostgreSQL Project Dependencies Installer
echo "========================================"
echo " PostgreSQL Project Dependencies Installer"
echo "========================================"
echo
echo "Установка компонентов для РАБОТЫ с программой"
echo "Без компиляции исходного кода"
echo

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Функции для красивого вывода
print_info() {
    echo -e "${CYAN}ℹ️  $1${NC}"
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

print_header() {
    echo -e "${BLUE}▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬${NC}"
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
        OS_VERSION=$VERSION_ID
    else
        OS=$(uname -s)
        OS_ID=$(uname -s | tr '[:upper:]' '[:lower:]')
    fi
}

# Функция установки пакета
install_package() {
    local package=$1
    local description=$2
    
    print_info "Установка $description ($package)..."
    
    case $OS_ID in
        ubuntu|debian|linuxmint)
            sudo apt update 2>/dev/null && sudo apt install -y "$package"
            ;;
        centos|rhel|fedora|rocky|almalinux)
            if command_exists dnf; then
                sudo dnf install -y "$package"
            else
                sudo yum install -y "$package"
            fi
            ;;
        arch|manjaro|endeavouros)
            sudo pacman -S --noconfirm "$package"
            ;;
        opensuse*|tumbleweed|leap)
            sudo zypper install -y "$package"
            ;;
        *)
            print_error "Неизвестный дистрибутив: $OS"
            return 1
            ;;
    esac
}

# Проверка и установка PostgreSQL клиента
install_postgresql_client() {
    print_header "POSTGRESQL КЛИЕНТ"
    
    if command_exists psql; then
        print_success "PostgreSQL клиент уже установлен: $(psql --version | head -n1)"
        return 0
    fi
    
    print_info "Установка PostgreSQL клиента..."
    
    case $OS_ID in
        ubuntu|debian|linuxmint)
            install_package "postgresql-client" "PostgreSQL клиент"
            ;;
        centos|rhel|fedora|rocky|almalinux)
            if command_exists dnf; then
                sudo dnf install -y postgresql
            else
                sudo yum install -y postgresql
            fi
            ;;
        arch|manjaro|endeavouros)
            install_package "postgresql" "PostgreSQL клиент"
            ;;
        opensuse*|tumbleweed|leap)
            install_package "postgresql" "PostgreSQL клиент"
            ;;
    esac
    
    if command_exists psql; then
        print_success "PostgreSQL клиент установлен: $(psql --version | head -n1)"
    else
        print_error "Не удалось установить PostgreSQL клиент"
    fi
}

# Проверка и установка библиотек для работы с PostgreSQL
install_postgresql_libs() {
    print_header "БИБЛИОТЕКИ POSTGRESQL"
    
    local lib_package=""
    
    case $OS_ID in
        ubuntu|debian|linuxmint)
            lib_package="libpq5"
            ;;
        centos|rhel|fedora|rocky|almalinux)
            lib_package="libpq"
            ;;
        arch|manjaro|endeavouros)
            lib_package="postgresql-libs"
            ;;
        opensuse*|tumbleweed|leap)
            lib_package="libpq5"
            ;;
    esac
    
    install_package "$lib_package" "Библиотеки PostgreSQL (libpq)"
}

# Проверка и установка зависимостей выполнения
install_runtime_dependencies() {
    print_header "ЗАВИСИМОСТИ ВЫПОЛНЕНИЯ"
    
    # Проверяем стандартные библиотеки C++
    print_info "Проверка стандартных библиотек C++..."
    
    case $OS_ID in
        ubuntu|debian|linuxmint)
            install_package "libstdc++6" "Стандартная библиотека C++"
            ;;
        centos|rhel|fedora|rocky|almalinux)
            install_package "libstdc++" "Стандартная библиотека C++"
            ;;
        arch|manjaro|endeavouros)
            # В Arch это обычно уже установлено
            print_success "Стандартные библиотеки C++ уже установлены"
            ;;
        opensuse*|tumbleweed|leap)
            install_package "libstdc++6" "Стандартная библиотека C++"
            ;;
    esac
}

# Создание файла запуска
create_launcher() {
    print_header "НАСТРОЙКА ЗАПУСКА ПРОГРАММЫ"
    
    # Проверяем существование исполняемого файла
    local executable=""
    
    if [ -f "build/bin/StudentManagementSystem" ]; then
        executable="build/bin/StudentManagementSystem"
    elif [ -f "build/StudentManagementSystem" ]; then
        executable="build/StudentManagementSystem"
    elif [ -f "StudentManagementSystem" ]; then
        executable="StudentManagementSystem"
    else
        print_warning "Исполняемый файл не найден в текущей директории"
        print_info "После скачивания программы поместите её в эту директорию"
        return 0
    fi
    
    # Делаем файл исполняемым
    chmod +x "$executable" 2>/dev/null
    
    # Создаем простой скрипт запуска
    cat > run_program.sh << 'EOF'
#!/bin/bash
echo "========================================"
echo " Запуск Student Management System"
echo "========================================"
echo

# Ищем исполняемый файл
if [ -f "build/bin/StudentManagementSystem" ]; then
    ./build/bin/StudentManagementSystem
elif [ -f "build/StudentManagementSystem" ]; then
    ./build/StudentManagementSystem
elif [ -f "StudentManagementSystem" ]; then
    ./StudentManagementSystem
else
    echo "Ошибка: Исполняемый файл не найден!"
    echo "Убедитесь, что программа находится в одной из директорий:"
    echo "  - build/bin/StudentManagementSystem"
    echo "  - build/StudentManagementSystem"
    echo "  - StudentManagementSystem (в текущей директории)"
    exit 1
fi
EOF
    
    chmod +x run_program.sh
    print_success "Создан скрипт запуска: run_program.sh"
    print_info "Для запуска программы выполните: ./run_program.sh"
}

# Создание файла проверки
create_check_script() {
    print_header "СКРИПТ ПРОВЕРКИ СИСТЕМЫ"
    
    cat > check_system.sh << 'EOF'
#!/bin/bash
echo "========================================"
echo " Проверка системы"
echo "========================================"
echo

# Цвета
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

check_command() {
    if command -v $1 &> /dev/null; then
        echo -e "${GREEN}✅ $1 установлен${NC}"
        return 0
    else
        echo -e "${RED}❌ $1 не установлен${NC}"
        return 1
    fi
}

echo "=== Проверка зависимостей ==="
check_command "psql"
check_command "ldd" && ldd --version | head -n1

echo
echo "=== Проверка библиотеки libpq ==="
if ldconfig -p | grep -q libpq; then
    echo -e "${GREEN}✅ libpq найдена в системе${NC}"
else
    echo -e "${YELLOW}⚠️  libpq не найдена в кэше ldconfig${NC}"
fi

echo
echo "=== Проверка исполняемого файла ==="
if [ -f "build/bin/StudentManagementSystem" ]; then
    echo -e "${GREEN}✅ Исполняемый файл найден: build/bin/StudentManagementSystem${NC}"
    echo "   Размер: $(du -h build/bin/StudentManagementSystem | cut -f1)"
    echo "   Права: $(ls -la build/bin/StudentManagementSystem | cut -d' ' -f1)"
elif [ -f "build/StudentManagementSystem" ]; then
    echo -e "${GREEN}✅ Исполняемый файл найден: build/StudentManagementSystem${NC}"
elif [ -f "StudentManagementSystem" ]; then
    echo -e "${GREEN}✅ Исполняемый файл найден: StudentManagementSystem${NC}"
else
    echo -e "${RED}❌ Исполняемый файл не найден${NC}"
fi

echo
echo "========================================"
echo " Для запуска программы выполните:"
echo "   ./run_program.sh"
echo "========================================"
EOF
    
    chmod +x check_system.sh
    print_success "Создан скрипт проверки: check_system.sh"
}

# Показать информацию о системе
show_system_info() {
    print_header "ИНФОРМАЦИЯ О СИСТЕМЕ"
    
    echo "Операционная система: $OS"
    echo "ID дистрибутива: $OS_ID"
    [ -n "$OS_VERSION" ] && echo "Версия: $OS_VERSION"
    echo "Архитектура: $(uname -m)"
    echo "Ядро: $(uname -r)"
    echo
}

# Показать справку
show_help() {
    cat << EOF
────────────────────────────────────────────────────
 Student Management System - Установка зависимостей
────────────────────────────────────────────────────

ЭТОТ СКРИПТ УСТАНАВЛИВАЕТ ТОЛЬКО ЗАВИСИМОСТИ
ДЛЯ РАБОТЫ С УЖЕ СКОМПИЛИРОВАННОЙ ПРОГРАММОЙ

📦 Что будет установлено:
──────────────────────────────
1. PostgreSQL клиент (psql)
2. Библиотеки PostgreSQL (libpq)
3. Стандартные библиотеки C++

🚀 Что будет создано:
──────────────────────
• run_program.sh - скрипт для запуска программы
• check_system.sh - скрипт проверки системы

⚠️  ВНИМАНИЕ:
─────────────
Этот скрипт НЕ компилирует программу!
Он только устанавливает необходимые компоненты
для работы с уже скомпилированной программой.

📁 Где должна быть программа:
─────────────────────────────
Поместите файл StudentManagementSystem в одну из папок:
  • build/bin/StudentManagementSystem
  • build/StudentManagementSystem  
  • ./StudentManagementSystem (текущая директория)

📋 Использование:
─────────────────
sudo ./install.sh     - полная установка
./check_system.sh     - проверка установки
./run_program.sh      - запуск программы

Для сборки программы используйте build_linux.sh
EOF
}

# Основная функция
main() {
    # Показываем справку
    show_help
    
    echo
    read -p "Продолжить установку зависимостей? [y/N] " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Установка отменена"
        exit 0
    fi
    
    # Определяем ОС
    detect_os
    show_system_info
    
    # Проверка прав
    if [ "$EUID" -ne 0 ]; then
        print_warning "Скрипт требует прав суперпользователя для установки пакетов"
        echo "Попробуйте запустить: sudo ./install.sh"
        exit 1
    fi
    
    # Устанавливаем компоненты
    install_postgresql_client
    install_postgresql_libs
    install_runtime_dependencies
    
    # Создаем скрипты
    create_launcher
    create_check_script
    
    print_header "✅ УСТАНОВКА ЗАВЕРШЕНА"
    echo
    print_success "Все зависимости установлены!"
    echo
    echo "📋 Дальнейшие действия:"
    echo "─────────────────────────────"
    echo "1. Поместите файл StudentManagementSystem в текущую директорию"
    echo "2. Проверьте систему: ./check_system.sh"
    echo "3. Запустите программу: ./run_program.sh"
    echo
    echo "🔧 Для проверки установки:"
    echo "   $ ./check_system.sh"
    echo
    echo "▶️  Для запуска программы:"
    echo "   $ ./run_program.sh"
    echo
    echo "⚠️  Если программа ещё не скомпилирована:"
    echo "   Запустите сборку: ./build_linux.sh"
}

# Запуск основной функции
main