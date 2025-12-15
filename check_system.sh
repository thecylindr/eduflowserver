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
