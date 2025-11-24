#!/bin/bash
# build.sh - Скрипт для сборки проекта

set -e  # Выход при ошибке

PROJECT_NAME="kunstkammer"
BUILD_DIR="build_tmp"
DEFAULT_BUILD_TYPE="RelWithDebInfo"

# Функция для проверки зависимостей
check_dependencies() {
    local missing_deps=()
    
    # Проверка компилятора C++
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        missing_deps+=("C++ compiler (g++ or clang++)")
    fi
    
    # Проверка CMake
    if ! command -v cmake &> /dev/null; then
        missing_deps+=("CMake")
    fi
    
    # Проверка Make
    if ! command -v make &> /dev/null; then
        missing_deps+=("GNU Make")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo "❌ Отсутствуют необходимые зависимости:"
        for dep in "${missing_deps[@]}"; do
            echo "   - $dep"
        done
        echo ""
        echo "Установите зависимости и повторите попытку."
        exit 1
    fi
    
    # Показываем версии инструментов
    echo "📋 Версии инструментов:"
    if command -v g++ &> /dev/null; then
        echo "   - g++: $(g++ --version | head -n1)"
    fi
    if command -v clang++ &> /dev/null; then
        echo "   - clang++: $(clang++ --version | head -n1)"
    fi
    echo "   - CMake: $(cmake --version | head -n1)"
    echo "   - Make: $(make --version | head -n1)"
    echo ""
}

# Функция для определения генератора CMake
detect_cmake_generator() {
    if command -v ninja &> /dev/null; then
        echo "Ninja"
    elif command -v make &> /dev/null; then
        echo "Unix Makefiles"
    else
        echo "❌ Не найден подходящий генератор сборки (make или ninja)"
        exit 1
    fi
}

# Функция для вывода справки
show_help() {
    echo "Использование: $0 [ОПЦИИ]"
    echo ""
    echo "ОПЦИИ:"
    echo "  -d, --debug      Сборка в режиме Debug"
    echo "  -r, --release    Сборка в режиме Release"
    echo "  -p, --profile    Сборка в режиме RelWithDebInfo (по умолчанию)"
    echo "  -s, --minsize    Сборка в режиме MinSizeRel"
    echo "  -c, --clean      Очистка перед сборкой"
    echo "  -j, --jobs N     Количество параллельных jobs (по умолчанию: nproc)"
    echo "  -g, --generator  Использовать конкретный генератор (make, ninja)"
    echo "  -v, --verbose    Подробный вывод"
    echo "  -h, --help       Показать эту справку"
    echo ""
    echo "ПРИМЕРЫ:"
    echo "  $0 -r           # Release сборка"
    echo "  $0 -d -c        # Debug сборка с очисткой"
    echo "  $0 -r -j 4      # Release сборка с 4 потоками"
    echo "  $0 -g ninja     # Сборка с использованием ninja"
}

# Парсинг аргументов
BUILD_TYPE="$DEFAULT_BUILD_TYPE"
CLEAN_BUILD=false
VERBOSE=false
JOBS=$(nproc)
GENERATOR=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -p|--profile)
            BUILD_TYPE="RelWithDebInfo"
            shift
            ;;
        -s|--minsize)
            BUILD_TYPE="MinSizeRel"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -g|--generator)
            GENERATOR="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Неизвестная опция: $1"
            show_help
            exit 1
            ;;
    esac
done

# Проверка зависимостей
echo "🔍 Проверка зависимостей..."
check_dependencies

# Определение генератора если не указан явно
if [ -z "$GENERATOR" ]; then
    GENERATOR=$(detect_cmake_generator)
fi

echo "🎯 Используемый генератор: $GENERATOR"

# Создание директории сборки
mkdir -p "$BUILD_DIR"

# Очистка если нужно
if [ "$CLEAN_BUILD" = true ]; then
    echo "🧹 Очистка предыдущей сборки..."
    rm -rf "$BUILD_DIR"/*
fi

# Конфигурация
echo "⚙️  Конфигурация сборки: $BUILD_TYPE"
cd "$BUILD_DIR"

CMAKE_CMD="cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -G \"$GENERATOR\" .."
if [ "$VERBOSE" = true ]; then
    echo "Выполняется: $CMAKE_CMD"
fi

eval $CMAKE_CMD

if [ $? -ne 0 ]; then
    echo "❌ Ошибка конфигурации CMake!"
    echo "Попробуйте следующие решения:"
    echo "1. Убедитесь, что установлен компилятор C++"
    echo "2. Попробуйте использовать другой генератор: $0 -g ninja"
    echo "3. Проверьте права доступа к директории"
    exit 1
fi

# Сборка
echo "🔨 Сборка проекта с $JOBS потоками..."
MAKE_CMD="cmake --build . --config $BUILD_TYPE -- -j$JOBS"
if [ "$VERBOSE" = true ]; then
    echo "Выполняется: $MAKE_CMD"
fi

eval $MAKE_CMD

if [ $? -ne 0 ]; then
    echo "❌ Ошибка сборки!"
    exit 1
fi

# Проверка результата
if [ -f "$PROJECT_NAME" ] || [ -f "$PROJECT_NAME.exe" ]; then
    echo "✅ Сборка завершена успешно!"
    echo "📦 Исполняемый файл: $BUILD_DIR/$PROJECT_NAME"
    
    # Показываем информацию о бинарнике
    echo ""
    echo "📊 Информация о бинарнике:"
    if [ -f "$PROJECT_NAME" ]; then
        file "$PROJECT_NAME"
        size "$PROJECT_NAME" 2>/dev/null || ls -lh "$PROJECT_NAME" | awk '{print "Size: " $5}'
    else
        ls -lh "$PROJECT_NAME.exe" | awk '{print "Size: " $5}'
    fi
else
    echo "❌ Исполняемый файл не найден после сборки!"
    echo "Проверьте вывод компилятора на наличие ошибок."
    exit 1
fi