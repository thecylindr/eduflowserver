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
