# Лабораторная работа 3: Проектирование и оптимизация реляционной базы данных

**Цель работы**: Получить практические навыки работы с PostgreSQL, проектирования схемы
БД, создания индексов и оптимизации запросов.

## Схема БД

Схематично связь таблиц выглядит так:

```text
┌────────────────────┐       ┌────────────────────┐       ┌────────────────────────┐
│       users        │       │      drivers       │       │         rides          │
├────────────────────┤       ├────────────────────┤       ├────────────────────────┤
│ id            PK   │◄──┐   │ id            PK   │       │ id                PK   │
│ login         UQ   │◄──┼───│ login         UQ FK│◄──┐   │ passenger_login   FK   │──► users.login
│ password_hash      │   │   │ user_id       UQ FK│───┘   │ driver_login      FK   │──► drivers.login
│ first_name         │   │   │ car_model          │       │ from_lat               │
│ last_name          │   │   │ car_number    UQ   │       │ from_lon               │
│ email              │   │   │ car_class          │       │ to_lat                 │
│ role               │   │   │ status             │       │ to_lon                 │
└────────────────────┘   │   └────────────────────┘       │ car_class              │
                         │                                │ status                 │
                         │                                │ price                  │
                         │                                │ created_at             │
                         └────────────────────────────────│                        │
                                                          └────────────────────────┘
```

Связи:
- `drivers.user_id` → `users.id`: один пользователь может иметь один водительский профиль.
- `drivers.login` → `users.login`: водительский профиль связан с учетной записью пользователя по логину.
- `rides.passenger_login` → `users.login`: один пользователь может создать много поездок.
- `rides.driver_login` → `drivers.login`: один водитель может принять много поездок.

### Таблица `users`

Назначение: хранение учетных данных и профиля пользователя.

| Колонка | Тип | Ограничения | Назначение |
|---|---|---|---|
| `id` | `TEXT` | `PRIMARY KEY` | Технический идентификатор пользователя |
| `login` | `TEXT` | `NOT NULL`, `UNIQUE` | Уникальный логин для входа и поиска |
| `password_hash` | `TEXT` | `NOT NULL` | Хэш пароля |
| `first_name` | `TEXT` | `NOT NULL` | Имя |
| `last_name` | `TEXT` | `NOT NULL` | Фамилия |
| `email` | `TEXT` | nullable | Email пользователя |
| `role` | `TEXT` | `NOT NULL`, `DEFAULT 'passenger'`, `CHECK` | Роль пользователя |

Допустимые роли:

```sql
CHECK (role IN ('passenger', 'driver'))
```


### Таблица `drivers`

Назначение: хранение водительского профиля, связанного с существующим
пользователем.

| Колонка | Тип | Ограничения | Назначение |
|---|---|---|---|
| `id` | `TEXT` | `PRIMARY KEY` | Технический идентификатор водителя |
| `user_id` | `TEXT` | `NOT NULL`, `UNIQUE`, `FK -> users(id)` | Связь с пользователем |
| `login` | `TEXT` | `NOT NULL`, `UNIQUE`, `FK -> users(login)` | Логин водителя |
| `car_model` | `TEXT` | `NOT NULL` | Модель автомобиля |
| `car_number` | `TEXT` | `NOT NULL`, `UNIQUE` | Номер автомобиля |
| `car_class` | `TEXT` | `NOT NULL`, `CHECK` | Класс автомобиля |
| `status` | `TEXT` | `NOT NULL`, `DEFAULT 'OFFLINE'`, `CHECK` | Текущий статус водителя |

Допустимые классы автомобиля:

```sql
CHECK (car_class IN ('economy', 'comfort', 'business'))
```

Допустимые статусы водителя:

```sql
CHECK (status IN ('FREE', 'BUSY', 'OFFLINE'))
```

Связи:
- `drivers.user_id -> users.id` гарантирует, что водитель не может существовать
  без пользователя.
- `drivers.login -> users.login` упрощает работу с текущим API, где водитель
  определяется логином из JWT.

`UNIQUE (user_id)` и `UNIQUE (login)` не позволяют зарегистрировать несколько
водительских профилей для одного пользователя.

## Таблица `rides`

Назначение: хранение заказов поездок. В одной таблице находятся и активные
заказы, и завершенная история.

| Колонка | Тип | Ограничения | Назначение |
|---|---|---|---|
| `id` | `TEXT` | `PRIMARY KEY` | Технический идентификатор поездки |
| `passenger_login` | `TEXT` | `NOT NULL`, `FK -> users(login)` | Пассажир, создавший заказ |
| `driver_login` | `TEXT` | nullable, `FK -> drivers(login)` | Назначенный водитель |
| `from_lat` | `DOUBLE PRECISION` | `NOT NULL`, `CHECK` | Широта точки отправления |
| `from_lon` | `DOUBLE PRECISION` | `NOT NULL`, `CHECK` | Долгота точки отправления |
| `to_lat` | `DOUBLE PRECISION` | `NOT NULL`, `CHECK` | Широта точки назначения |
| `to_lon` | `DOUBLE PRECISION` | `NOT NULL`, `CHECK` | Долгота точки назначения |
| `car_class` | `TEXT` | `NOT NULL`, `CHECK` | Требуемый класс автомобиля |
| `status` | `TEXT` | `NOT NULL`, `DEFAULT 'CREATED'`, `CHECK` | Статус поездки |
| `price` | `NUMERIC(10,2)` | `NOT NULL`, `CHECK (price >= 0)` | Стоимость поездки |
| `created_at` | `TIMESTAMPTZ` | `NOT NULL`, `DEFAULT now()` | Время создания |

Координаты ограничены допустимыми диапазонами:

```sql
CHECK (from_lat BETWEEN -90 AND 90)
CHECK (from_lon BETWEEN -180 AND 180)
CHECK (to_lat BETWEEN -90 AND 90)
CHECK (to_lon BETWEEN -180 AND 180)
```

Допустимые статусы поездки:

```sql
CHECK (status IN ('CREATED', 'ACCEPTED', 'COMPLETED', 'CANCELLED'))
```

`driver_login` может быть `NULL`, потому что новый заказ сначала создается без
назначенного водителя. После принятия заказа поле заполняется логином водителя,
а статус меняется на `ACCEPTED`.

## Индексы

PK и UNIQUE автоматически создают индексы:
- `users_pkey` для `users.id`
- `users_login_key` для `users.login`
- `drivers_pkey` для `drivers.id`
- `drivers_user_id_key` для `drivers.user_id`
- `drivers_login_key` для `drivers.login`
- `drivers_car_number_key` для `drivers.car_number`
- `rides_pkey` для `rides.id`

Дополнительные индексы из `schema.sql`:

| Индекс | Таблица | Назначение |
|---|---|---|
| `users_first_name_trgm_idx` | `users` | Поиск по маске имени через `ILIKE` |
| `users_last_name_trgm_idx` | `users` | Поиск по маске фамилии через `ILIKE` |
| `drivers_status_class_idx` | `drivers` | Поиск доступных водителей нужного класса |
| `rides_passenger_login_idx` | `rides` | FK/JOIN и фильтрация по пассажиру |
| `rides_driver_login_idx` | `rides` | FK/JOIN и фильтрация по водителю |
| `rides_status_created_at_idx` | `rides` | Получение активных заказов с сортировкой |
| `rides_passenger_created_at_idx` | `rides` | История поездок пассажира |
| `rides_driver_created_at_idx` | `rides` | История поездок водителя |
| `rides_id_status_idx` | `rides` | Быстрое обновление состояния поездки |

Для trigram-индексов используется расширение:

```sql
CREATE EXTENSION IF NOT EXISTS pg_trgm;
```

## Основные запросы

Создание пользователя:

```sql
INSERT INTO users (id, login, password_hash, first_name, last_name, email, role)
VALUES ($1, $2, $3, $4, $5, $6, 'passenger');
```

Поиск по логину:

```sql
SELECT id, login, first_name, last_name, email, role
FROM users
WHERE login = $1
LIMIT 1;
```

Поиск по маске имени/фамилии:

```sql
SELECT id, login, first_name, last_name, email, role
FROM users
WHERE first_name ILIKE $1 || '%' OR last_name ILIKE $1 || '%'
ORDER BY login;
```

Активные заказы:

```sql
SELECT id, passenger_login, driver_login, from_lat, from_lon, to_lat, to_lon,
       car_class, status, price, created_at
FROM rides
WHERE status = 'CREATED'
ORDER BY created_at DESC;
```

Принятие заказа:

```sql
UPDATE rides
SET status = 'ACCEPTED', driver_login = $2
WHERE id = $1 AND status = 'CREATED';
```

История поездок пользователя:

```sql
SELECT id, passenger_login, driver_login, from_lat, from_lon, to_lat, to_lon,
       car_class, status, price, created_at
FROM rides
WHERE passenger_login = $1 OR driver_login = $1
ORDER BY created_at DESC;
```

Полный набор запросов находится в `database/queries.sql`.


## Запуск

Для того чтобы запустить сервис в режиме Postgres, необходимо передать ENV переменую

```bash
TAXI_DB_BACKEND=postgres docker compose up --build -d
```

Основные переменные:
- `TAXI_DB_BACKEND=postgres` — все сервисы используют PostgreSQL
- `TAXI_DB_BACKEND=sqlite` — все сервисы используют SQLite
- `TAXI_POSTGRES_DSN` — строка подключения PostgreSQL
- `TAXI_SQLITE_USERS_DB` — путь к SQLite-файлу пользователей
- `TAXI_SQLITE_RIDES_DB` — путь к SQLite-файлу поездок

По умолчанию в `docker-compose.yml` выбран PostgreSQL:

```yaml
TAXI_DB_BACKEND: ${TAXI_DB_BACKEND:-postgres}
```

Запуск в SQLite-режиме:

```bash
TAXI_DB_BACKEND=sqlite docker compose up --build -d
```

Запуск только `user-service` на отдельном backend возможен через более точный
переключатель:

```bash
TAXI_USER_DB_BACKEND=postgres docker compose up --build -d
```

## Файлы


- [schema.sql](../database/schema.sql) — создания схемы БД
- [data.sql](../database/data.sql) — тестовые данные
- [queries.sql](../database/queries.sql) — запросы для операций API
- [optimization.md](../database/optimization.md) — описание оптимизаций с планами выполнения

