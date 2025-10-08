
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
```

## Доступные фильтры для карточек:
* board_id - ID доски
* lane_id - ID лейна
* column_id - ID колонки
* type - тип карточки
* state - состояние
* member_id - ID участника
* owner_id - ID владельца
* archived - архивные (true/false)

### Пример использования

```bash
# Получить все карточки с фильтрами
./program --cards-filter "board_id=123,type=task" --config config.json

# Получить всех пользователей
./program --users-list --config config.json

# Получить конкретного пользователя
./program --get-user 12345 --config config.json

# Получить карточки по различным критериям
./program --cards-filter "lane_id=678,state=active" --config config.json

# Получить все карточки с пагинацией и сортировкой
./program --cards-list --page-size 50 --sort-by updated --sort-order desc

# Получить карточки с фильтрами и пагинацией
./program --cards-filter "board_id=123,state=active" --page-size 100

# Получить всех пользователей с пагинацией
./program --users-list --page-size 200

# Получить все доски
./program --boards-list

# С настройками по умолчанию (кэширование и rate limiting включены)
./program --get-card CARD-123

# Без кэширования
./program --cards-list --no-cache

# Без rate limiting
./program --users-list --no-rate-limit

# С кастомными лимитами
./program --cards-list --rate-limit-per-minute 30 --rate-limit-per-hour 500

# Показать статистику
./program --cache-stats --rate-limit-stats

# Очистить кэш
./program --clear-cache

# Комбинированное использование
./program --cards-list --no-cache --rate-limit-per-minute 10
```



