# Лабораторная работа 4: Проектирование и работа с MongoDB

**Цель работы**: Получить практические навыки работы с MongoDB, проектирования
документной модели данных, выполнения CRUD операций и валидации схем.

## Документная модель

В MongoDB используются три коллекции: `users`, `drivers`, `rides`.

```text
┌──────────────────────────┐       ┌──────────────────────────┐       ┌────────────────────────────┐
│          users           │       │         drivers          │       │           rides            │
├──────────────────────────┤       ├──────────────────────────┤       ├────────────────────────────┤
│ _id                 PK   │       │ _id                 PK   │       │ _id                   PK   │
│ id                  UQ   │       │ id                  UQ   │       │ id                    UQ   │
│ login               UQ   │◄──┐   │ user_login          REF  │──► users.login              │
│ password_hash            │   └───│ login               UQ   │◄──┐   │ passenger_login       REF  │──► users.login
│ first_name               │       │ car { ... }         EMB  │   └───│ driver_login          REF  │──► drivers.login
│ last_name                │       │ car_class                │       │ route { from, to }    EMB  │
│ email                    │       │ status                   │       │ car_class                  │
│ role                     │       │ rating                   │       │ status                     │
│ rating                   │       │ shifts [ ... ]      EMB  │       │ price                      │
│ created_at               │       └──────────────────────────┘       │ created_at                 │
│ profile { ... }     EMB  │                                          │ events [ ... ]        EMB  │
│ saved_places [ ... ] EMB │                                          └────────────────────────────┘
│ payment_methods [ ] EMB  │
└──────────────────────────┘
```

Обозначения:
- `PK` - MongoDB автоматически создает `_id` как первичный идентификатор документа.
- `UQ` - уникальное бизнес-поле, для которого создан unique index.
- `REF` - логическая ссылка на документ другой коллекции.
- `EMB` - embedded object или embedded array внутри документа.

MongoDB не создает внешние ключи как PostgreSQL, поэтому связи
`drivers.user_login`, `rides.passenger_login`, `rides.driver_login` являются
логическими references. Целостность таких связей контролируется приложением.

## Коллекция `users`

Назначение: хранение аккаунтов пассажиров и пользователей, которые могут стать
водителями.

Основные поля:

| Поле | Тип MongoDB | Назначение |
|---|---|---|
| `_id` | `ObjectId` | Внутренний идентификатор MongoDB |
| `id` | `String` | Бизнес-идентификатор пользователя, например `u-1` |
| `login` | `String` | Уникальный логин пользователя |
| `password_hash` | `String` | Хэш пароля |
| `first_name` | `String` | Имя |
| `last_name` | `String` | Фамилия |
| `email` | `String` | Email |
| `role` | `String` | Роль: `passenger` или `driver` |
| `rating` | `Number` | Рейтинг пользователя от 0 до 5 |
| `created_at` | `Date` | Дата создания аккаунта |
| `profile` | `Object` | Embedded-профиль: телефон, локаль |
| `saved_places` | `Array<Object>` | Embedded-массив сохраненных адресов |
| `payment_methods` | `Array<String>` | Embedded-массив способов оплаты |

Почему embedded:
- `profile` принадлежит только одному пользователю и обычно читается вместе с аккаунтом.
- `saved_places` нужны только внутри профиля пользователя.
- `payment_methods` являются настройкой конкретного пользователя.

Вынос этих полей в отдельные коллекции усложнил бы чтение профиля без пользы для
данной предметной области.

## Коллекция `drivers`

Назначение: хранение водительского профиля.

Основные поля:

| Поле | Тип MongoDB | Назначение |
|---|---|---|
| `_id` | `ObjectId` | Внутренний идентификатор MongoDB |
| `id` | `String` | Бизнес-идентификатор водителя |
| `user_login` | `String` | Reference на `users.login` |
| `login` | `String` | Уникальный логин водителя, используется API |
| `car` | `Object` | Embedded-объект автомобиля |
| `car.model` | `String` | Модель автомобиля |
| `car.number` | `String` | Номер автомобиля |
| `car.class` | `String` | Класс автомобиля |
| `car.year` | `Number` | Год выпуска |
| `car_class` | `String` | Класс автомобиля для быстрой фильтрации |
| `status` | `String` | Статус: `FREE`, `BUSY`, `OFFLINE` |
| `rating` | `Number` | Рейтинг водителя |
| `shifts` | `Array<Object>` | Embedded-массив смен водителя |

Почему reference:
- `drivers.user_login` ссылается на `users.login`, потому что водительский
  профиль существует как расширение отдельного пользовательского аккаунта.

Почему embedded:
- `car` хранится внутри водителя, потому что текущий автомобиль читается вместе
  с профилем водителя.
- `shifts` хранится массивом, потому что это история, принадлежащая конкретному
  водителю.

## Коллекция `rides`

Назначение: хранение активных заказов и истории поездок.

Основные поля:

| Поле | Тип MongoDB | Назначение |
|---|---|---|
| `_id` | `ObjectId` | Внутренний идентификатор MongoDB |
| `id` | `String` | Бизнес-идентификатор поездки |
| `passenger_login` | `String` | Reference на `users.login` |
| `driver_login` | `String` / `null` | Reference на `drivers.login`; `null`, пока заказ не принят |
| `route` | `Object` | Embedded-маршрут поездки |
| `route.from` | `Object` | Координаты точки отправления |
| `route.to` | `Object` | Координаты точки назначения |
| `car_class` | `String` | Требуемый класс автомобиля |
| `status` | `String` | Статус: `CREATED`, `ACCEPTED`, `COMPLETED`, `CANCELLED` |
| `price` | `Decimal128` | Стоимость поездки |
| `created_at` | `Date` | Дата создания заказа |
| `events` | `Array<Object>` | Embedded-массив событий жизненного цикла поездки |

Почему reference:
- `passenger_login` является ссылкой на пользователя, потому что один пассажир
  может иметь много поездок.
- `driver_login` является ссылкой на водителя, потому что один водитель может
  принять много поездок.

Почему embedded:
- `route` принадлежит только одной поездке.
- `events` являются историей конкретной поездки и обычно читаются вместе с ней.

## Тестовые данные

Скрипт [data.js](/Users/denis/system_design/mongodb/data.js) создает тестовый
набор данных:

- 10 документов в `users`
- 10 документов в `drivers`
- 10 документов в `rides`

Используются разные типы данных MongoDB:
- `String`: `login`, `first_name`, `status`
- `Number`: `rating`, `car.year`
- `Decimal128`: `price`
- `Date`: `created_at`, события поездок
- `Object`: `profile`, `car`, `route`
- `Array`: `saved_places`, `payment_methods`, `shifts`, `events`
- `null`: `driver_login` у поездки, которая еще не принята водителем

## CRUD-запросы

Скрипт [queries.js](/Users/denis/system_design/mongodb/queries.js) содержит
операции для сценариев системы заказа такси.

Create:
- создание нового пассажира через `db.users.insertOne`
- создание новой поездки через `db.rides.insertOne`

Read:
- поиск пользователя по логину через `$eq`
- поиск по маске имени или фамилии через `$regex` и `$or`
- поиск доступных водителей через `$ne` и `$in`
- поиск поездок по диапазону цены и даты через `$and`, `$gt`, `$lt`
- просмотр активных заказов
- просмотр истории поездок пассажира или водителя

Update:
- принятие поездки водителем через `$set` и `$push`
- завершение поездки через `$set` и `$push`
- добавление способа оплаты без дублей через `$addToSet`
- удаление сохраненного адреса через `$pull`

Delete:
- удаление тестовой поездки через `deleteOne`

## Валидация схемы

В [validation.js](/Users/denis/system_design/mongodb/validation.js) создана
валидация `$jsonSchema` для коллекции `users`.

Проверяются:
- обязательные поля: `id`, `login`, `password_hash`, `first_name`, `last_name`, `role`, `created_at`
- типы данных полей
- формат `id` по шаблону `^u-[0-9]+$`
- длина и формат `login`
- длина `password_hash` ровно 64 символа
- допустимые значения `role`: `passenger`, `driver`
- диапазон `rating` от 0 до 5
- тип `created_at` как `date`

В конце скрипта выполняется тестовая вставка невалидного пользователя. MongoDB
отклоняет документ, а скрипт выводит сообщение:

```text
Validation check passed: invalid user was rejected
```

## Индексы

Индексы создаются в [validation.js](/Users/denis/system_design/mongodb/validation.js).

| Индекс | Назначение |
|---|---|
| `users.login` unique | Быстрый поиск пользователя и проверка уникальности логина |
| `users.first_name, users.last_name` | Поиск пользователей по имени и фамилии |
| `drivers.login` unique | Быстрый поиск водителя по логину |
| `drivers.status, drivers.car_class` | Поиск свободных водителей нужного класса |
| `rides.status, rides.created_at` | Список активных заказов и сортировка по дате |
| `rides.passenger_login, rides.created_at` | История поездок пассажира |
| `rides.driver_login, rides.created_at` | История поездок водителя |

## Подключение API к MongoDB

MongoDB backend подключен для `user-service`.

Реализованные операции API:
- создание пользователя
- поиск пользователя по логину
- поиск пользователей по маске имени или фамилии
- логин пользователя
- повышение роли пользователя до водителя на уровне репозитория

Переключатели backend:

| Переменная | Значение |
|---|---|
| `TAXI_USER_DB_BACKEND=mongo` | `user-service` использует MongoDB |
| `TAXI_USER_DB_BACKEND=postgres` | `user-service` использует PostgreSQL |
| `TAXI_USER_DB_BACKEND=sqlite` | `user-service` использует SQLite |
| `TAXI_MONGO_URI` | Строка подключения к MongoDB |
| `TAXI_MONGO_DB` | Имя базы данных |

В [docker-compose.yml](/Users/denis/system_design/docker-compose.yml) задана
строка подключения:

```text
mongodb://taxi:taxi@mongo:27017/taxi?authSource=admin
```

## Запуск 

Запуск только MongoDB:

```bash
docker compose up --build -d mongo
```

Запуск API с MongoDB backend для `user-service`:

```bash
TAXI_USER_DB_BACKEND=mongo docker compose up --build -d
```


## Файлы

- [schema_design.md](../mongodb/schema_design.md) - описание проектирования документной модели с обоснованием embedded/reference связей
- [data.js](../mongodb/data.js) - скрипт с тестовыми данными
- [queries.js](../mongodb/queries.js) - MongoDB CRUD-запросы для операций системы
- [validation.js](../mongodb/validation.js) - создание `$jsonSchema` validation и индексов