# Оптимизация запросов

Основные частые сценарии API: список активных заказов, история поездок
пользователя и атомарная смена статуса поездки. PostgreSQL автоматически
создает индексы для PK и UNIQUE, но индексы для FK и фильтров добавлены явно.

## Индексы

- `users.login` создается через UNIQUE и используется для авторизации, поиска
  пользователя и связей с `drivers`/`rides`.
- `users_first_name_trgm_idx`, `users_last_name_trgm_idx` ускоряют поиск по
  маске имени/фамилии через `ILIKE`.
- `drivers_status_class_idx` нужен для выборки свободных водителей нужного
  класса автомобиля.
- `rides_passenger_login_idx`, `rides_driver_login_idx` покрывают FK и JOIN по
  участникам поездки.
- `rides_status_created_at_idx` ускоряет получение активных заказов с
  сортировкой по времени создания.
- `rides_passenger_created_at_idx`, `rides_driver_created_at_idx` ускоряют
  историю поездок пассажира или водителя.
- `rides_id_status_idx` помогает обновлениям вида `WHERE id = ? AND status = ?`.

## Сравнение планов

Активные поездки до `rides_status_created_at_idx`:

```text
Seq Scan on rides
  Filter: (status = 'CREATED')
Sort
  Sort Key: created_at DESC
```

После индекса:

```text
Bitmap Index Scan on rides_status_created_at_idx
  Index Cond: (status = 'CREATED')
Bitmap Heap Scan on rides
```

История поездок до индексов по участникам:

```text
Seq Scan on rides
  Filter: ((passenger_login = 'ivan') OR (driver_login = 'ivan'))
Sort
  Sort Key: created_at DESC
```

После индексов PostgreSQL читает меньше строк через bitmap/index scan:

```text
Bitmap Index Scan on rides_passenger_created_at_idx
  Index Cond: (passenger_login = 'ivan')
Bitmap Heap Scan on rides
```

Принятие поездки до `rides_id_status_idx`:

```text
Index Scan using rides_pkey on rides
  Index Cond: (id = 'r-1')
  Filter: (status = 'CREATED')
```

После композитного индекса:

```text
Index Scan using rides_id_status_idx on rides
  Index Cond: ((id = 'r-1') AND (status = 'CREATED'))
```

Пример запуска EXPLAIN:

```bash
docker compose exec -T postgres psql -U taxi -d taxi -c "EXPLAIN (ANALYZE, BUFFERS) SELECT id, passenger_login, driver_login, created_at FROM rides WHERE status = 'CREATED' ORDER BY created_at DESC LIMIT 50;"
```
