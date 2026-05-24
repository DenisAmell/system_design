# Курс «Программная инженерия»

## Вариант 16 — Система заказа такси (аналог Uber)

### Сущности предметной области

- **Пользователь** (пассажир)
- **Водитель**
- **Поездка**

### Требуемое API

- Создание нового пользователя
- Поиск пользователя по логину
- Поиск пользователя по маске имя/фамилия
- Регистрация водителя
- Создание заказа поездки
- Получение активных заказов
- Принятие заказа водителем
- Получение истории поездок пользователя
- Завершение поездки

## Быстрый старт

```bash
docker compose up --build -d
```

После запуска поднимаются два сервиса:

| URL                     | Назначение                                                  |
|-------------------------|-------------------------------------------------------------|
| <http://localhost:8080> | REST API                       |
| <http://localhost:8081> | Swagger UI  |

\
По умолчанию API использует PostgreSQL. Для запуска старого SQLite-режима:

```bash
TAXI_DB_BACKEND=sqlite docker compose up --build -d
```

Запуск в mongo режиме:

```bash
TAXI_USER_DB_BACKEND=mongo docker compose up --build -d
```

Основные переменные:
- `TAXI_DB_BACKEND=postgres` — все сервисы используют PostgreSQL
- `TAXI_DB_BACKEND=sqlite` — все сервисы используют SQLite
- `TAXI_USER_DB_BACKEND=mongo` - `user-service` использует MongoDB 
- `TAXI_POSTGRES_DSN` — строка подключения PostgreSQL
- `TAXI_SQLITE_USERS_DB` — путь к SQLite-файлу пользователей
- `TAXI_SQLITE_RIDES_DB` — путь к SQLite-файлу поездок




## Документация


- [lab1](./docs/lab1.md) — Структурное описание архитектуры (Structurizr).
- [lab2](./docs/lab2.md) — Проектирование, реализация, документирование и тестирование REST API.
- [lab3](./docs/lab3.md) — Проектирование PostgreSQL БД, индексы, оптимизация и подключение API.
- [lab4](./docs/lab4.md) — Проектирование MongoDB, CRUD, валидация схем и подключение API.
- [lab5](./docs/lab5.md) — Оптимизация производительности через кеширование и rate
limiting.
- [lab6](./docs/lab6.md) — Проектирование Event-Driven архитектуры, Kafka и CQRS.

## Файлы
- [workspace.dsl](./workspace.dsl) — описание архитектуры в формате Structurizr DSL.
- [openapi.yaml](./openapi.yaml) — спецификация API.
- [performance_design](./performance_design.md) - описание стратегии кеширования и rate limiting
- [event_driven_design.md](/Users/denis/system_design/event-driven/event_driven_design.md) - описание Event-Driven архитектуры
- [event_catalog.md](/Users/denis/system_design/event-driven/event_catalog.md) - каталог событий

SQL-артефакты находятся в папке [database](./database):
`schema.sql`, `data.sql`, `queries.sql`, `optimization.md`.

MongoDB-артефакты лнаходятся в папке [mongodb](./mongodb):
`schema_design.md`, `data.js`, `queries.js`, `validation.js`, `README.md`.
