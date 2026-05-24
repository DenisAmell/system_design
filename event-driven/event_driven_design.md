# Event-Driven архитектура системы заказа такси

## Выбор брокера сообщений

Для лабораторной работы выбран **Apache Kafka**.

Причины выбора:

- Kafka уже заложена в архитектурном описании проекта в `workspace.dsl` и
  `docs/lab1.md`.
- userver имеет штатный модуль `userver::kafka`, поэтому producer и consumer
  реализованы без сторонних Python/Node-скриптов.
- Kafka хорошо подходит для event log: события поездок можно хранить,
  перечитывать и использовать для построения read-моделей.

Используемый топик:

```text
taxi.events
```

## Команды и события

Команда - это намерение выполнить действие. Команда приходит в API и
обрабатывается сервисом-владельцем данных.

Событие - это факт, который уже произошел. Событие публикуется после успешного
изменения состояния.

| Команда | Сервис-обработчик | Событие после успешной обработки |
|---|---|---|
| `CreateUser` | `user-service` | `UserRegistered` |
| `RegisterDriver` | `driver-service` | `DriverRegistered` |
| `SetDriverStatus` | `driver-service` | `DriverStatusChanged` |
| `CreateRide` | `ride-service` | `RideCreated` |
| `AcceptRide` | `ride-service` | `RideAccepted` |
| `CompleteRide` | `ride-service` | `RideCompleted` |
| `CancelRide` | `ride-service` | `RideCancelled` |
| `AuthorizePayment` | `payment-service` | `PaymentAuthorized` / `PaymentFailed` |
| `SendNotification` | `notification-service` | `NotificationRequested` |

## Производители событий

| Producer | Какие события публикует |
|---|---|
| `user-service` | `UserRegistered` |
| `driver-service` | `DriverRegistered`, `DriverStatusChanged` |
| `ride-service` | `RideCreated`, `RideAccepted`, `RideCompleted`, `RideCancelled` |
| `matching-service` | `DriverMatched` |
| `payment-service` | `PaymentAuthorized`, `PaymentFailed` |
| `event-service` | Демонстрационные события лабораторной работы |

## Потребители событий

| Consumer | Какие события потребляет | Для чего |
|---|---|---|
| `matching-service` | `RideCreated` | Подбор свободного водителя |
| `notification-service` | `UserRegistered`, `RideCreated`, `DriverMatched`, `RideAccepted`, `RideCompleted`, `RideCancelled`, `PaymentFailed` | Push/email/SMS уведомления |
| `read-model-service` | Все доменные события | Построение read-моделей для быстрых запросов |
| `analytics-service` | Все события поездок и оплат | Аналитика заказов, спроса и выручки |
| `event-service` | `taxi.events` | Демонстрационный consumer для лабораторной |

## Формат сообщения

Все события передаются в формате JSON.

Общий envelope:

```json
{
  "event_id": "evt-1",
  "event_type": "RideCreated",
  "event_version": 1,
  "occurred_at": "2026-05-24T12:00:00Z",
  "producer": "ride-service",
  "aggregate_type": "ride",
  "aggregate_id": "r-42",
  "payload": {}
}
```

Назначение полей:

| Поле | Назначение |
|---|---|
| `event_id` | Уникальный идентификатор события для дедупликации |
| `event_type` | Тип события |
| `event_version` | Версия схемы payload |
| `occurred_at` | Время возникновения события |
| `producer` | Сервис, который опубликовал событие |
| `aggregate_type` | Тип агрегата: `user`, `driver`, `ride`, `payment` |
| `aggregate_id` | Идентификатор агрегата |
| `payload` | Данные конкретного события |

Ключ Kafka-сообщения (`message key`) равен `aggregate_id`. Это сохраняет порядок
событий внутри одного агрегата, например внутри одной поездки.

## Поток событий для заказа поездки

```text
Пассажир
   │
   │ POST /v1/rides
   ▼
ride-service ── сохраняет поездку CREATED ──► Kafka: RideCreated
   │                                              │
   │ HTTP 201                                     ├──► matching-service
   │                                              ├──► notification-service
   │                                              ├──► read-model-service
   │                                              └──► analytics-service
   ▼
Пассажир видит созданный заказ
```

Поток принятия заказа:

```text
Водитель
   │
   │ POST /v1/rides/{id}/accept
   ▼
ride-service ── меняет поездку на ACCEPTED ──► Kafka: RideAccepted
   │                                              │
   │ синхронно обновляет driver-service           ├──► notification-service
   │                                              ├──► read-model-service
   │                                              └──► analytics-service
   ▼
Водитель видит принятый заказ
```

Поток завершения поездки:

```text
Водитель
   │
   │ POST /v1/rides/{id}/complete
   ▼
ride-service ── меняет поездку на COMPLETED ──► Kafka: RideCompleted
   │                                               │
   │ освобождает водителя                          ├──► notification-service
   │                                               ├──► read-model-service
   │                                               └──► analytics-service
   ▼
История поездок обновлена
```

## Гарантии доставки

Для лабораторной работы используется гарантия **at-least-once**.

Это означает:

- producer ожидает подтверждение доставки сообщения в Kafka;
- consumer коммитит offset после успешной обработки пачки сообщений;
- если consumer упадет до commit, событие может быть обработано повторно;
- обработчики должны быть идемпотентными.

Для идемпотентности используется поле `event_id`. Consumer может хранить
обработанные `event_id` и пропускать дубликаты.

Exactly-once в данной лабораторной работе не используется, потому что она
требует более сложной настройки транзакций Kafka и согласования с базой данных.
Для системы заказа такси достаточно at-least-once при идемпотентной обработке.

## CQRS

CQRS применим к системе заказа такси, потому что операции записи и чтения имеют
разный профиль нагрузки.

Команды изменяют состояние:

- создать пользователя;
- зарегистрировать водителя;
- создать поездку;
- принять поездку;
- завершить поездку;
- изменить статус водителя.

Запросы читают подготовленную модель:

- найти пользователя по логину;
- найти пользователей по маске имени;
- получить активные заказы;
- получить историю поездок пользователя;
- получить доступных водителей.

Write model:

- основная модель в сервисах `user-service`, `driver-service`, `ride-service`;
- хранится в PostgreSQL/SQLite/MongoDB в зависимости от выбранного backend;
- изменяется только командами.

Read model:

- денормализованные представления для быстрых запросов;
- может храниться в отдельной таблице PostgreSQL, MongoDB-коллекции или Redis;
- обновляется асинхронно через события из Kafka.

Пример синхронизации:

```text
CreateRide command
  └─► ride-service пишет поездку в write DB
      └─► публикует RideCreated
          └─► read-model-service обновляет список активных заказов
```

Такой подход уменьшает связность сервисов: write-сервис не обязан синхронно
вызывать все сервисы, которым интересен факт создания поездки.

## Реализация лабораторного producer/consumer

Для демонстрации добавлен сервис `event-service`:

- producer: HTTP handler `POST /v1/events/demo`;
- broker: Kafka topic `taxi.events`;
- consumer: userver component `event-consumer`, который читает `taxi.events` и
  пишет обработанные события в лог.

Producer использует штатный компонент userver:

```cpp
userver::kafka::ProducerComponent
```

Consumer использует штатный компонент userver:

```cpp
userver::kafka::ConsumerComponent
```

Kafka-настройки передаются через `SECDIST_CONFIG` в формате, который ожидает
userver Kafka module.
