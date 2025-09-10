
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
