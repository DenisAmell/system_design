# Лабораторная работа 2: Разработка REST API сервиса

**Цель работы**: Получить практические навыки разработки REST API сервиса с
использованием принципов REST, обработкой HTTP запросов, реализацией аутентификации
и документированием API

## 1. Проектирование REST API

### Базовый URL

`http://localhost:8080/v1`

### Ресурсы

| Ресурс | URL | Комментарий |
|--------|-----|-------------|
| Пользователи | `/users` | Пассажиры и водители (роль в теле). |
| Водители     | `/drivers` | Регистрация водителя поверх существующего пользователя. |
| Поездки      | `/rides` | Заказы и история. |



### API

| # | Метод | URL | Назначение | Auth | Коды ответа |
|---|-------|-----|------------|------|-------------|
| 1 | POST   | `/v1/users`                       | Создать пользователя           | нет | `201`, `400`, `409` |
| 2 | GET    | `/v1/users?login={login}`         | Найти по логину                | JWT | `200`, `401`, `404` |
| 3 | GET    | `/v1/users?nameMask={mask}`       | Поиск по маске ФИО             | JWT | `200`, `400`, `401` |
| 4 | POST   | `/v1/drivers`                     | Регистрация водителя           | JWT | `201`, `400`, `401`, `404`, `409` |
| 5 | POST   | `/v1/rides`                       | Создать заказ поездки          | JWT | `201`, `400`, `401` |
| 6 | GET    | `/v1/rides?status=active`         | Активные заказы                | JWT | `200`, `401`, `403` |
| 7 | POST   | `/v1/rides/{id}/accept`           | Принять заказ водителем        | JWT | `200`, `401`, `403`, `404`, `409` |
| 8 | POST   | `/v1/rides/{id}/complete`         | Завершить поездку              | JWT | `200`, `401`, `403`, `404`, `409` |
| 9 | GET    | `/v1/users/{login}/rides`         | История поездок пользователя   | JWT | `200`, `401`, `404` |

### Используемые HTTP-коды
- `200 OK` — успешный GET / действие.
- `201 Created` — успешное создание ресурса (с заголовком `Location`).
- `400 Bad Request` — некорректное тело/параметры.
- `401 Unauthorized` — нет или невалидный токен.
- `403 Forbidden` — у роли нет прав (пассажир пробует принять заказ и т.п.).
- `404 Not Found` — ресурс не существует.
- `409 Conflict` — конфликт состояния (логин занят, заказ уже принят и т.п.).

### DTO (Request / Response)

#### User
```json
// Request: POST /v1/users
{
  "login":      "ivan",
  "password":   "secret",
  "first_name": "Иван",
  "last_name":  "Иванов",
  "email":      "ivan@example.com"
}

// Response: 201 Created
{
  "id":         "u-1",
  "login":      "ivan",
  "first_name": "Иван",
  "last_name":  "Иванов",
  "email":      "ivan@example.com",
  "role":       "passenger"
}
```

#### Driver
```json
// Request: POST /v1/drivers
{
  "login":       "ivan",
  "car_model":   "Hyundai Solaris",
  "car_number":  "А123ВС777",
  "car_class":   "economy"
}

// Response: 201 Created
{
  "id":          "d-1",
  "user_id":     "u-1",
  "car_model":   "Hyundai Solaris",
  "car_number":  "А123ВС777",
  "car_class":   "economy",
  "status":      "OFFLINE"
}
```

#### Ride
```json
// Request: POST /v1/rides
{
  "from": { "lat": 55.7558, "lon": 37.6173 },
  "to":   { "lat": 55.7522, "lon": 37.6156 },
  "car_class": "economy"
}

// Response: 201 Created
{
  "id":             "r-1",
  "passenger_login":"ivan",
  "driver_login":   null,
  "from":           { "lat": 55.7558, "lon": 37.6173 },
  "to":             { "lat": 55.7522, "lon": 37.6156 },
  "car_class":      "economy",
  "status":         "CREATED",
  "price":          250.0,
  "created_at":     "2026-05-19T12:00:00Z"
}
```

#### Error
```json
{ "error": "user_not_found", "message": "User 'ivan' not found" }
```


### Запуск
```bash
docker compose up --build
```

### Примеры запросов (curl)
```bash
# 1. Зарегистрировать пассажира и водителя (без токена)
curl -X POST http://localhost:8080/v1/users -H 'Content-Type: application/json' \
  -d '{"login":"ivan","password":"secret","first_name":"Иван","last_name":"Иванов","email":"i@e.com"}'
curl -X POST http://localhost:8080/v1/users -H 'Content-Type: application/json' \
  -d '{"login":"petr","password":"secret","first_name":"Петр","last_name":"Петров","email":"p@e.com"}'

# 2. Получить JWT для каждого
TOKEN_IVAN=$(curl -s -X POST http://localhost:8080/v1/login -H 'Content-Type: application/json' \
  -d '{"login":"ivan","password":"secret"}' | jq -r .token)
TOKEN_PETR=$(curl -s -X POST http://localhost:8080/v1/login -H 'Content-Type: application/json' \
  -d '{"login":"petr","password":"secret"}' | jq -r .token)

# 3. Зарегистрировать petr как водителя
curl -X POST http://localhost:8080/v1/drivers \
  -H "Authorization: Bearer $TOKEN_PETR" -H 'Content-Type: application/json' \
  -d '{"car_model":"Solaris","car_number":"А1","car_class":"economy"}'

# 4. Пассажир создаёт поездку
curl -X POST http://localhost:8080/v1/rides \
  -H "Authorization: Bearer $TOKEN_IVAN" -H 'Content-Type: application/json' \
  -d '{"from":{"lat":55.75,"lon":37.61},"to":{"lat":55.76,"lon":37.62},"car_class":"economy"}'

# 5. Найти пользователя по логину
curl "http://localhost:8080/v1/users?login=ivan" -H "Authorization: Bearer $TOKEN_IVAN"

# 6. Поиск пользователя по маске ФИО
curl "http://localhost:8080/v1/users?nameMask=Ив" -H "Authorization: Bearer $TOKEN_IVAN"

# 7. Активные заказы (только для водителя)
curl "http://localhost:8080/v1/rides?status=active" -H "Authorization: Bearer $TOKEN_PETR"

# 8. Принять заказ
curl -X POST http://localhost:8080/v1/rides/r-1/accept   -H "Authorization: Bearer $TOKEN_PETR"

# 9. Завершить поездку
curl -X POST http://localhost:8080/v1/rides/r-1/complete -H "Authorization: Bearer $TOKEN_PETR"

# 10. История пользователя
curl http://localhost:8080/v1/users/ivan/rides -H "Authorization: Bearer $TOKEN_IVAN"
```


## Документирование API

Swagger UI доступен на  http://localhost:8081 

Полная спецификация API описана в [`openapi.yaml`](../openapi.yaml)
## Тестирование

Тесты на pytest лежат в [`tests/`](../tests/) и запускаются поверх живого сервиса

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install -r tests/requirements.txt
docker compose up -d taxi-service
pytest tests/ -v
```
