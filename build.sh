#!/bin/bash
# build.sh - Скрипт для сборки проекта

set -e  # Выход при ошибке

PROJECT_NAME="kunstkammer"
BUILD_DIR="build"
DEFAULT_BUILD_TYPE="RelWithDebInfo"

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
    echo "  -h, --help       Показать эту справку"
    echo ""
    echo "ПРИМЕРЫ:"
    echo "  $0 -r           # Release сборка"
    echo "  $0 -d -c        # Debug сборка с очисткой"
    echo "  $0 -r -j 4      # Release сборка с 4 потоками"
}

# Парсинг аргументов
BUILD_TYPE="$DEFAULT_BUILD_TYPE"
CLEAN_BUILD=false
JOBS=$(nproc)

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
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..

# Сборка
echo "🔨 Сборка проекта с $JOBS потоками..."
make -j"$JOBS"

# Проверка результата
if [ -f "$PROJECT_NAME" ]; then
    echo "✅ Сборка завершена успешно!"
    echo "📦 Исполняемый файл: $BUILD_DIR/$PROJECT_NAME"
    
    # Показываем информацию о бинарнике
    echo ""
    echo "📊 Информация о бинарнике:"
    file "$PROJECT_NAME"
    size "$PROJECT_NAME" 2>/dev/null || ls -lh "$PROJECT_NAME" | awk '{print "Size: " $5}'
else
    echo "❌ Ошибка сборки!"
    exit 1
fi