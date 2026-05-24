# Лабораторная работа 5: Оптимизация производительности через кеширование и rate limiting

**Цель работы**: Получить практические навыки проектирования систем с учетом
производительности, реализации кеширования и rate limiting.

## Подробное обоснование стратегии
Подробное обоснование стратегии, выбора алгоритмов и метрик — в [`performance_design.md`](../performance_design.md)

## Что добавилось в систему

```text
   ┌──────────────┐         ┌──────────────┐
   │  user-svc    │────┐    │ driver-svc   │────┐
   └──────────────┘    │    └──────────────┘    │
                       ▼                        ▼
                   ┌──────────────────────────────┐
                   │           Redis              │  cache + rate limiter
                   │  (taxi-redis :6379)          │
                   └──────────────────────────────┘
```

- Один Redis-сервер на весь кластер.
- Кеш живёт в namespace `cache:*`, rate-limit — в `rl:*`.
- Никаких подписок/pub-sub — только KV-операции (`GET`, `SET EX`, `DEL`, `INCR`, `EXPIRE`).


### Кешируемые endpoint'ы

| Endpoint | Cache key | TTL | Инвалидация |
|----------|-----------|-----|-------------|
| `GET /v1/users?login=…`         | `cache:user:by_login:<login>`   | 60 с | `POST /v1/users` создаёт запись → `DEL` ключа |
| `GET /internal/users/{login}`   | тот же namespace                | 60 с | то же |
| `GET /internal/drivers/{login}` | `cache:driver:by_login:<login>` | 30 с | `POST /v1/drivers` + `POST /internal/drivers/{login}/status` → `DEL` |

Обе ручки выставляют заголовок `X-Cache: HIT|MISS`, чтобы можно было считать hit rate из access logs:

```bash
docker compose logs api-gateway | grep -oE 'X-Cache:[A-Z]+' | sort | uniq -c
```

### Rate-limited endpoint

| Endpoint | Лимит | Алгоритм | Бакет |
|----------|-------|----------|-------|
| `POST /v1/login` | 5 / 60 с | Fixed Window Counter | `rl:login:<X-Real-IP>:<window_start>` |

На каждом ответе (200/400/401/429) проставляются заголовки `X-RateLimit-Limit`, `X-RateLimit-Remaining`, `X-RateLimit-Reset`. На 429 возвращается тело `{"error": "rate_limited", "message": "too many login attempts, retry later"}`.
