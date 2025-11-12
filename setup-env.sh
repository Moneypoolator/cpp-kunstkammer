#!/bin/bash
# setup.sh - Установка зависимостей

set -e

echo "🔍 Проверка и установка зависимостей..."

# Определение ОС
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    if command -v apt &> /dev/null; then
        # Ubuntu/Debian
        echo "📦 Установка зависимостей для Ubuntu/Debian..."
        sudo apt update
        sudo apt install -y build-essential cmake make g++ clang libboost-all-dev libssl-dev
        
    elif command -v yum &> /dev/null; then
        # CentOS/RHEL
        echo "📦 Установка зависимостей для CentOS/RHEL..."
        sudo yum groupinstall -y "Development Tools"
        sudo yum install -y cmake make gcc-c++ clang boost-devel openssl-devel
        
    elif command -v dnf &> /dev/null; then
        # Fedora
        echo "📦 Установка зависимостей для Fedora..."
        sudo dnf groupinstall -y "Development Tools"
        sudo dnf install -y cmake make gcc-c++ clang boost-devel openssl-devel
    fi
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    echo "📦 Установка зависимостей для macOS..."
    if ! command -v brew &> /dev/null; then
        echo "❌ Homebrew не установлен. Установите Homebrew с https://brew.sh"
        exit 1
    fi
    brew install cmake make boost openssl
    
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    # Windows (MSYS2/Cygwin)
    echo "📦 Установка зависимостей для Windows (MSYS2)..."
    if command -v pacman &> /dev/null; then
        pacman -S --noconfirm mingw-w64-x86_64-toolchain \
                            mingw-w64-x86_64-cmake \
                            mingw-w64-x86_64-boost \
                            mingw-w64-x86_64-openssl
    else
        echo "❌ MSYS2 не найден. Установите MSYS2 с https://www.msys2.org/"
        exit 1
    fi
else
    echo "❌ Неподдерживаемая операционная система: $OSTYPE"
    exit 1
fi

echo "✅ Зависимости установлены успешно!"
echo "🚀 Теперь можно выполнить сборку: ./build.sh -r"