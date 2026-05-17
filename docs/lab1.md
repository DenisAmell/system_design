# Лабораторная работа 1: Документирование архитектуры в Structurizr

Цель работы: Получить навык в описании архитектуры в стиле Architecture As A Code и проектировании
системы «сверху вниз».

## Роли пользователей
| Роль | Описание |
|------|----------|
| **Пассажир** | Регистрируется, заказывает поездки, видит историю. |
| **Водитель** | Регистрируется как водитель, получает активные заказы, принимает их, завершает поездки. |
| **Администратор** | Управляет пользователями, водителями, мониторит поездки. |

## Внешние системы
| Система | Назначение |
|---------|------------|
| Платёжная система (Stripe / ЮKassa) | Списание стоимости поездки. |
| SMS-шлюз | Подтверждение регистрации, уведомления. |
| Push-сервис (APNs / FCM) | Push-уведомления о новых заказах, статусах поездки. |
| Сервис карт (Yandex / Google Maps) | Маршрут, расчёт стоимости и ETA, расстояния. |

## Контейнеры системы
| Контейнер | Технология | Ответственность |
|-----------|------------|------------------|
| Мобильное приложение пассажира | iOS / Android | UI пассажира. |
| Мобильное приложение водителя  | iOS / Android | UI водителя. |
| Веб-панель администратора      | React / TS | UI администратора. |
| API Gateway                    | C++ Userver | Единая точка входа, аутентификация, маршрутизация. |
| User Service                   | C++ Userver | Регистрация/поиск пользователей. |
| Driver Service                 | C++ Userver | Регистрация и статусы водителей, геопозиции. |
| Ride Service                   | C++ Userver | Заказы, статусы, история поездок. |
| Matching Service               | C++ Userver | Подбор ближайшего свободного водителя. |
| Notification Service           | C++ Userver | Доставка push/SMS. |
| User Database                  | PostgreSQL  | Учётные данные пользователей и водителей. |
| Ride Database                  | PostgreSQL  | Заказы и история поездок. |
| Cache / Geo Store              | Redis       | Активные заказы, геоиндекс водителей. |
| Message Broker                 | Apache Kafka | Шина асинхронных событий между сервисами. |



### Карта связей между контейнерами

| Источник | Назначение | Тип | Назначение взаимодействия |
|---|---|---|---|
| Мобильные приложения / Веб-админка | API Gateway | HTTPS/JSON | Все клиентские запросы. |
| API Gateway | User / Driver / Ride Service | HTTP/JSON | Маршрутизация после аутентификации. |
| Ride Service | Driver Service | HTTP/JSON | Резервирование/освобождение водителя. |
| User Service | User DB | TCP | Чтение/запись пользователей. |
| Driver Service | User DB | TCP | Чтение/запись профилей водителей. |
| Driver Service | Redis (Geo Store) | RESP | Обновление геопозиции водителя. |
| Ride Service | Ride DB | TCP | CRUD по заказам и история. |
| Ride Service | Redis | RESP | Список активных заказов. |
| Matching Service | Redis | RESP | Поиск ближайшего водителя. |
| Ride Service | Kafka | Kafka protocol | Публикация `RideCreated`, `RideAccepted`, `RideCompleted`. |
| Matching Service | Kafka | Kafka protocol | Подписка на `RideCreated`, публикация `DriverCandidateSelected`. |
| Notification Service | Kafka | Kafka protocol | Подписка на события поездок. |
| Notification Service | Push-сервис / SMS-шлюз | HTTPS | Доставка push/SMS. |
| Ride Service | Платёжная система | HTTPS/REST | Списание оплаты по завершении поездки. |
| Ride Service / Matching Service | Сервис карт | HTTPS/REST | Маршрут, ETA, расстояние до пассажира. |

### Основные сценарии взаимодействия

**1. Регистрация пользователя**
Мобильное приложение → API Gateway → **User Service** → User DB.
User Service валидирует логин, хэширует пароль, сохраняет запись и возвращает JWT.

**2. Поиск пользователя по логину / маске ФИО**
Клиент → API Gateway → **User Service** → User DB (индексированный поиск). Результат возвращается синхронно.

**3. Регистрация водителя**
Driver App → API Gateway → **Driver Service**. Driver Service создаёт запись водителя в User DB (роль «driver»), сохраняет атрибуты автомобиля, выставляет начальный статус `OFFLINE`.

**4. Создание заказа поездки** 
Passenger App → API Gateway → **Ride Service** → Maps (стоимость/ETA) → Ride DB + Redis → Kafka `RideCreated` → **Matching Service** ищет водителя в Redis-геоиндексе → Kafka `DriverCandidateSelected` → **Notification Service** → Push водителю.

**5. Принятие заказа водителем**
Driver App → API Gateway → **Ride Service** → синхронный вызов **Driver Service** для перевода водителя в `BUSY` → обновление статуса заказа в Ride DB → Kafka `RideAccepted` → **Notification Service** → Push пассажиру «Водитель найден».

**6. Получение активных заказов**
Driver App опрашивает API Gateway → **Ride Service**, который читает активные заказы из Redis и фильтрует по геопозиции водителя.

**7. Завершение поездки**
Driver App → API Gateway → **Ride Service** → меняет статус заказа на `COMPLETED` в Ride DB → синхронно вызывает **Платёжную систему** для списания → освобождает водителя в **Driver Service** (`FREE`) → Kafka `RideCompleted` → **Notification Service** уведомляет обе стороны.

**8. Получение истории поездок пользователя**
Клиент → API Gateway → **Ride Service** → Ride DB (выборка по `user_id` с пагинацией).

## Архитектурно значимый сценарий
**«Создание заказа и принятие водителем»** — выбран как наиболее значимый, поскольку затрагивает большинство контейнеров и демонстрирует асинхронное взаимодействие через брокер сообщений и взаимодействие с внешним сервисом карт и push-уведомлений.

### Пошаговая последовательность взаимодействия

#### Фаза 1. Создание заказа

- Пассажир → Passenger App → API Gateway → **Ride Service** (`POST /rides`).
- Ride Service считает маршрут и стоимость через **Maps Service**, сохраняет заказ в Ride DB (`CREATED`) и кладёт в Redis.
- Ride Service публикует событие `RideCreated` в Kafka и синхронно отвечает приложению `201 Created`.

#### Фаза 2. Подбор водителя

- **Matching Service** получает `RideCreated` из Kafka.
- По геоиндексу в Redis находит ближайшего свободного водителя и публикует `DriverCandidateSelected`.

#### Фаза 3. Уведомление водителя

- **Notification Service** получает `DriverCandidateSelected` и отправляет push водителю через Push-сервис.

#### Фаза 4. Принятие заказа

- Водитель нажимает «Принять» → Driver App → API Gateway → **Ride Service** (`POST /rides/{id}/accept`).
- Ride Service синхронно резервирует водителя в **Driver Service** (`BUSY`), меняет статус заказа на `ACCEPTED` в Ride DB и публикует `RideAccepted` в Kafka.

#### Фаза 5. Уведомление пассажира

- **Notification Service** получает `RideAccepted` и шлёт пассажиру push «Водитель найден».

