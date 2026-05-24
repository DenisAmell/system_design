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

По умолчанию API использует PostgreSQL. Для запуска старого SQLite-режима:

```bash
TAXI_DB_BACKEND=sqlite docker compose up --build -d
```

SQL-артефакты лабораторной 3 находятся в папке [database](./database):
`schema.sql`, `data.sql`, `queries.sql`, `optimization.md`.

MongoDB-артефакты лабораторной 4 находятся в папке [mongodb](./mongodb):
`schema_design.md`, `data.js`, `queries.js`, `validation.js`, `README.md`.
Mongo backend для API подключен к `user-service` и включается так:

```bash
TAXI_USER_DB_BACKEND=mongo docker compose up --build -d
```

## Документация


- [lab1](./docs/lab1.md) — Структурное описание архитектуры (Structurizr).
- [lab2](./docs/lab2.md) — Проектирование, реализация, документирование и тестирование REST API.
- [lab3](./docs/lab3.md) — Проектирование PostgreSQL БД, индексы, оптимизация и подключение API.
- [lab4](./docs/lab4.md) — Проектирование MongoDB, CRUD, валидация схем и подключение API.

## Файлы
- [workspace.dsl](./workspace.dsl) — описание архитектуры в формате Structurizr DSL.
- [openapi.yaml](./openapi.yaml) — спецификация API.
