# Лабораторная работа 6: Проектирование Event-Driven архитектуры

**Цель работы**: спроектировать событийно-ориентированную архитектуру для
системы заказа такси, описать события и команды, выбрать брокер сообщений,
применить CQRS и реализовать простой producer/consumer.


## Выбор брокера

Для задания выбран **Apache Kafka**.

Причины:
- Kafka уже описана в архитектуре проекта как message broker.
- userver поддерживает Kafka через штатные компоненты `userver::kafka`.
- Kafka подходит для event log: события можно хранить, читать несколькими
  consumer groups и использовать для построения CQRS read-моделей.

## Общая схема

```text
┌──────────────┐      ┌──────────────┐      ┌────────────────────┐
│ API Gateway  │─────►│ write service│─────►│ Kafka: taxi.events │
└──────────────┘      └──────────────┘      └─────────┬──────────┘
       ▲                       │                       │
       │                       │                       ▼
       │                       │              ┌──────────────────┐
       │                       │              │ event consumers  │
       │                       │              ├──────────────────┤
       │                       │              │ matching-service │
       │                       │              │ notification     │
       │                       │              │ read-model       │
       │                       │              │ analytics        │
       │                       │              └──────────────────┘
       │                       ▼
       │              ┌────────────────┐
       └──────────────│ write database │
                      └────────────────┘
```

Write-сервис сначала сохраняет изменение в своей базе, а затем публикует
событие в Kafka. Другие сервисы подписываются на события и обновляют свои
модели независимо.

## Команды и события

| Команда | Сервис | Событие |
|---|---|---|
| `CreateUser` | `user-service` | `UserRegistered` |
| `RegisterDriver` | `driver-service` | `DriverRegistered` |
| `SetDriverStatus` | `driver-service` | `DriverStatusChanged` |
| `CreateRide` | `ride-service` | `RideCreated` |
| `AcceptRide` | `ride-service` | `RideAccepted` |
| `CompleteRide` | `ride-service` | `RideCompleted` |
| `CancelRide` | `ride-service` | `RideCancelled` |

## Producer и consumer

Producer:
- `user-service`
- `driver-service`
- `ride-service`
- демонстрационный `event-service`

Consumer:
- `matching-service`
- `notification-service`
- `read-model-service`
- `analytics-service`
- демонстрационный `event-service`

В лабораторной реализации добавлен [event-service](/Users/denis/system_design/services/event-service):

- `POST /v1/events/demo` публикует событие в Kafka;
- `event-consumer` читает события из `taxi.events`;
- consumer пишет обработанные события в лог.

## Формат события

```json
{
  "event_id": "evt-1",
  "event_type": "RideCreated",
  "event_version": 1,
  "occurred_at": "2026-05-24T12:00:00Z",
  "producer": "ride-service",
  "aggregate_type": "ride",
  "aggregate_id": "r-1",
  "payload": {}
}
```

Ключ Kafka-сообщения равен `aggregate_id`, чтобы события одного агрегата
попадали в один partition и сохраняли порядок.

## Гарантии доставки

Используется гарантия **at-least-once**:

- producer ожидает подтверждение доставки в Kafka;
- consumer коммитит offset после успешной обработки;
- при сбое событие может прийти повторно;
- обработчики должны быть идемпотентными.

Для дедупликации используется `event_id`.

## CQRS

CQRS применим к системе заказа такси.

Write model:
- принимает команды;
- проверяет бизнес-правила;
- изменяет состояние в основной базе;
- публикует события.

Read model:
- обслуживает запросы чтения;
- хранит денормализованные представления;
- обновляется асинхронно из событий Kafka.

Пример:

```text
CreateRide
  └─► ride-service пишет CREATED поездку
      └─► Kafka публикует RideCreated
          └─► read-model-service обновляет список активных заказов
```

## Запуск

Запуск Kafka и event-service:

```bash
docker compose up --build -d kafka kafka-init event-service
```

Полный запуск проекта:

```bash
docker compose up --build -d
```
## Файлы

- [event_driven_design.md](/Users/denis/system_design/event-driven/event_driven_design.md) - описание Event-Driven архитектуры
- [event_catalog.md](/Users/denis/system_design/event-driven/event_catalog.md) - каталог событий