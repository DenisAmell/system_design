# Каталог событий

Все события публикуются в Kafka topic `taxi.events` в JSON-формате.

Общий envelope:

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

Гарантия доставки для всех событий: **at-least-once**.

## `UserRegistered`

Производитель: `user-service`

Потребители:
- `notification-service`
- `read-model-service`
- `analytics-service`

Payload:

```json
{
  "user_id": "u-1",
  "login": "ivan",
  "first_name": "Ivan",
  "last_name": "Ivanov",
  "role": "passenger",
  "email": "ivan@example.com"
}
```

Назначение: уведомить систему о создании нового пользователя.

## `DriverRegistered`

Производитель: `driver-service`

Потребители:
- `notification-service`
- `read-model-service`
- `analytics-service`

Payload:

```json
{
  "driver_id": "d-1",
  "user_login": "petr",
  "car_model": "Hyundai Solaris",
  "car_number": "A123AA777",
  "car_class": "economy",
  "status": "OFFLINE"
}
```

Назначение: уведомить систему о регистрации водительского профиля.

## `DriverStatusChanged`

Производитель: `driver-service`

Потребители:
- `matching-service`
- `read-model-service`
- `analytics-service`

Payload:

```json
{
  "driver_login": "petr",
  "old_status": "BUSY",
  "new_status": "FREE",
  "changed_at": "2026-05-24T12:10:00Z"
}
```

Назначение: обновить доступность водителя для подбора и read-моделей.

## `RideCreated`

Производитель: `ride-service`

Потребители:
- `matching-service`
- `notification-service`
- `read-model-service`
- `analytics-service`

Payload:

```json
{
  "ride_id": "r-1",
  "passenger_login": "ivan",
  "from": { "lat": 55.7558, "lon": 37.6173 },
  "to": { "lat": 55.7522, "lon": 37.6156 },
  "car_class": "economy",
  "status": "CREATED",
  "price": 350.00
}
```

Назначение: запустить подбор водителя и обновить список активных заказов.

## `DriverMatched`

Производитель: `matching-service`

Потребители:
- `notification-service`
- `read-model-service`
- `analytics-service`

Payload:

```json
{
  "ride_id": "r-1",
  "driver_login": "petr",
  "match_score": 0.96,
  "estimated_arrival_minutes": 4
}
```

Назначение: сообщить, что для поездки найден кандидат-водитель.

## `RideAccepted`

Производитель: `ride-service`

Потребители:
- `notification-service`
- `read-model-service`
- `analytics-service`

Payload:

```json
{
  "ride_id": "r-1",
  "passenger_login": "ivan",
  "driver_login": "petr",
  "status": "ACCEPTED",
  "accepted_at": "2026-05-24T12:03:00Z"
}
```

Назначение: уведомить пассажира и обновить read-модель поездки.

## `RideCompleted`

Производитель: `ride-service`

Потребители:
- `notification-service`
- `read-model-service`
- `analytics-service`
- `payment-service`

Payload:

```json
{
  "ride_id": "r-1",
  "passenger_login": "ivan",
  "driver_login": "petr",
  "status": "COMPLETED",
  "price": 350.00,
  "completed_at": "2026-05-24T12:25:00Z"
}
```

Назначение: завершить поездку в read-модели, уведомить участников и инициировать
оплату.

## `RideCancelled`

Производитель: `ride-service`

Потребители:
- `notification-service`
- `read-model-service`
- `analytics-service`

Payload:

```json
{
  "ride_id": "r-1",
  "passenger_login": "ivan",
  "driver_login": "petr",
  "status": "CANCELLED",
  "reason": "passenger_cancelled",
  "cancelled_at": "2026-05-24T12:05:00Z"
}
```

Назначение: убрать заказ из активных списков и уведомить участников.

## `PaymentAuthorized`

Производитель: `payment-service`

Потребители:
- `notification-service`
- `read-model-service`
- `analytics-service`

Payload:

```json
{
  "payment_id": "p-1",
  "ride_id": "r-1",
  "amount": 350.00,
  "currency": "RUB",
  "status": "AUTHORIZED"
}
```

Назначение: подтвердить успешную оплату поездки.

## `PaymentFailed`

Производитель: `payment-service`

Потребители:
- `notification-service`
- `read-model-service`
- `analytics-service`

Payload:

```json
{
  "payment_id": "p-1",
  "ride_id": "r-1",
  "amount": 350.00,
  "currency": "RUB",
  "status": "FAILED",
  "reason": "insufficient_funds"
}
```

Назначение: уведомить систему о проблеме оплаты.

## `NotificationRequested`

Производитель: `notification-service`

Потребители:
- `analytics-service`

Payload:

```json
{
  "notification_id": "n-1",
  "recipient_login": "ivan",
  "channel": "push",
  "template": "ride_accepted",
  "status": "REQUESTED"
}
```

Назначение: зафиксировать факт постановки уведомления в отправку.
