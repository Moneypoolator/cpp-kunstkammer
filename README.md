# Kaiten API Client

C++ клиент для работы с API системы управления задачами Kaiten. Поддерживает создание карточек, получение данных с пагинацией, кэширование и rate limiting.

## Возможности

* Создание карточек из JSON файла или командной строки
* Получение карточек с фильтрацией и пагинацией
* Работа с пользователями и досками
* Автоматическое кэширование запросов
* Rate limiting для соблюдения лимитов API
* Подробная статистика и логирование
* Поддержка SSL/TLS соединений

## Сборка из исходного кода

### Установка зависимостей

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake libboost-system-dev libboost-program-options-dev libssl-dev
```

#### CentOS/RHEL

```bash
sudo yum install gcc-c++ cmake boost-devel openssl-devel
```

#### macOS

```bash
brew install cmake boost openssl
```

### Сборка проекта

```bash
# Клонирование репозитория
git clone <repository-url>
cd kaiten-client

# Создание директории для сборки
mkdir build && cd build

# Конфигурация проекта
cmake ..

# Сборка
make -j$(nproc)

# Или для Debug сборки
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## Конфигурация

Создайте файл config.json:

```json
{
  "Token": "your-api-token",
  "BaseURL": "https://your-company.kaiten.ru/api",
  "LogLevel": "info",
  "BoardID": "123",
  "ColumnID": "456",
  "LaneID": "789",
  "Tags": ["tag1", "tag2"]
}
```

## Использование

### Базовые команды

```bash
# Получить справку
./kaiten-client --help

# Создать карточки из JSON файла
./kaiten-client --tasks tasks.json --config config.json

# Создать одну карточку
./kaiten-client --create-card "Заголовок задачи" --type "Тип" --size 3 --tags "тег1,тег2"

# Получить информацию о карточке
./kaiten-client --get-card "CARD-12345"

# Получить список всех карточек
./kaiten-client --cards-list

# Получить карточки с фильтрацией
./kaiten-client --cards-filter "board_id=123,state=active,archived=false"

# Получить список пользователей
./kaiten-client --users-list

# Получить информацию о пользователе
./kaiten-client --get-user "123"

# Получить список досок
./kaiten-client --boards-list
```

### Параметры командной строки

Основные параметры
* --config <file> - путь к файлу конфигурации (по умолчанию: config.json)
* --help, -h - показать справку

Режимы работы
* --tasks <file> - создать карточки из JSON файла
* --create-card <title> - создать одну карточку
* --get-card <number> - получить карточку по номеру
* --cards-list - получить список карточек
* --cards-filter <filters> - фильтрация карточек
* --users-list - получить список пользователей
* --get-user <id> - получить пользователя по ID
* --boards-list - получить список досок

Параметры создания карточек
* --type <type> - тип карточки
* --size <number> - размер карточки
* --tags <tags> - теги через запятую

Управление кэшированием
* --no-cache - отключить кэширование
* --cache-stats - показать статистику кэша
* --clear-cache - очистить все кэши

Управление rate limiting
* --no-rate-limit - отключить rate limiting
* --rate-limit-stats - показать статистику rate limiting
* --rate-limit-per-minute <number> - лимит запросов в минуту (по умолчанию: 60)
* --rate-limit-per-hour <number> - лимит запросов в час (по умолчанию: 1000)
* --request-interval <ms> - минимальный интервал между запросами (по умолчанию: 100мс)

Пагинация
* --page-size <number> - размер страницы (по умолчанию: 100)
* --sort-by <field> - поле для сортировки
* --sort-order <order> - порядок сортировки (asc/desc)


### Формат JSON для создания задач

```json
{
  "schedule": {
    "tasks": [
      {
        "title": "Название задачи 1",
        "type": "Тип задачи",
        "size": 3,
        "tags": ["тег1", "тег2"]
      },
      {
        "title": "Название задачи 2", 
        "type": "Другой тип",
        "size": 5,
        "tags": ["тег3"]
      }
    ]
  }
}
```


### Примеры использования

Создание нескольких карточек из файла

```bash
./kaiten-client --tasks weekly-tasks.json --config config.json --no-cache
```

Получение карточек с фильтрацией

```bash
./kaiten-client --cards-filter "board_id=123,state=active,archived=false" --page-size 50
```

Создание одной карточки

```bash
./kaiten-client --create-card "Срочная задача" --type "Баг" --size 5 --tags "срочно,production" --config config.json
```

Мониторинг производительности

```bash
./kaiten-client --cards-list --cache-stats --rate-limit-stats
```

### Формат фильтров

Фильтры задаются в формате key1=value1,key2=value2 :
* board_id - ID доски
* lane_id - ID лейна
* column_id - ID колонки
* owner_id - ID владельца
* member_id - ID участника
* type_id - ID типа карточки
* type - название типа
* state - состояние карточки
* archived - архивные (true/false)
* blocked - заблокированные (true/false)
* asap - срочные (true/false)
* search - полнотекстовый поиск
* created_after, created_before - фильтр по дате создания
* updated_after, updated_before - фильтр по дате обновления









### Пример использования

```bash
# Получить карточку по номеру
./kunstkammer --get-card "60127" --config config.json

# Создать карточку
./kunstkammer --create-card "Новая задача" --type "task" --size 3 --tags "urgent,backend"

# Список карточек
./kunstkammer --cards-list

# Создание из файла задач
./kunstkammer --tasks tasks.json

# Получить все карточки с фильтрами
./kunstkammer --cards-filter "board_id=123,type=task" --config config.json

# Получить всех пользователей
./kunstkammer --users-list --config config.json

# Получить конкретного пользователя
./kunstkammer --get-user 12345 --config config.json

# Получить карточки по различным критериям
./kunstkammer --cards-filter "lane_id=678,state=active" --config config.json

# Получить все карточки с пагинацией и сортировкой
./kunstkammer --cards-list --page-size 50 --sort-by updated --sort-order desc

# Получить карточки с фильтрами и пагинацией
./kunstkammer --cards-filter "board_id=123,state=active" --page-size 100

# Получить всех пользователей с пагинацией
./kunstkammer --users-list --page-size 200

# Получить все доски
./kunstkammer --boards-list

# С настройками по умолчанию (кэширование и rate limiting включены)
./kunstkammer --get-card CARD-123

# Без кэширования
./kunstkammer --cards-list --no-cache

# Без rate limiting
./kunstkammer --users-list --no-rate-limit

# С кастомными лимитами
./kunstkammer --cards-list --rate-limit-per-minute 30 --rate-limit-per-hour 500

# Показать статистику
./kunstkammer --cache-stats --rate-limit-stats

# Очистить кэш
./kunstkammer --clear-cache

# Комбинированное использование
./kunstkammer --cards-list --no-cache --rate-limit-per-minute 10
```



