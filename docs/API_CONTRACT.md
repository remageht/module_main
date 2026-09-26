# API Gateway v1 — контракт интеграции module_main с другими модулями
# Шлюз: HTTP + JSON. Версия контракта: 1.0.0

## 1. Базовые принципы

- `module_main` — единая точка входа (API-шлюз). Клиенты НЕ ходят в модули напрямую.
- Все ответы — JSON, `Content-Type: application/json`, соединение закрывается (`Connection: close`).
- Формат ошибки: `{"error":"<code>","message":"<human readable>"}`.
- Секреты только в env. Токены никогда не пишутся в логи целиком.

## 2. Публичные эндпоинты шлюза

| Метод | Путь | Auth | Описание |
|-------|------|------|----------|
| GET | `/health` | нет | Liveness для Docker/LB. `{"status":"ok","version":"1.0.0","upstreams":{...}}` |
| GET/POST/PUT/PATCH/DELETE | `/api/<area>/...` | Bearer JWT | Авторизация по scope + проксирование в модуль |

`<area>` — один из: `users`, `courses`, `quests`, `tests`, `attempts`, `answers`.

Неизвестный путь → `404 {"error":"not_found"}`.
Метод не из списка → `405 {"error":"method_not_allowed"}`.
Тело больше `MAX_BODY_BYTES` (default 1 MiB) → `413 {"error":"body_too_large"}`.
Флуд выше `RATE_LIMIT_PER_MIN` (default 100 req/min/IP) → `429 {"error":"rate_limited"}`.

## 3. Auth: JWT HS256

- Заголовок: `Authorization: Bearer <token>`.
- Проверка: подпись `JWT_SECRET`, срок `exp` (если есть). Если задан `JWT_ISSUER` — поле `iss` обязано совпасть.
- Нет/битый/протухший токен → `401 {"error":"unauthorized"}`.
- Валидный токен, но нет права → `403 {"error":"forbidden"}`.

### 3.1. Модель прав (scopes)

Шлюз требует scope вида `<area>:read` (GET) или `<area>:write` (POST/PUT/PATCH/DELETE),
например `users:read`, `courses:write`.

Источники прав в токене (достаточно одного):
1. `permissions`: [<scope>, ...] — точное совпадение (`users:read`), либо
   гранулярное право из старой схемы (`user:list:read`, `course:info:write`, ...):
   засчитывается, если начинается с префикса `<area-singular>:` (`user:`, `course:`, ...).
2. `roles`: ["admin", ...] — роль `admin` открывает всё.

Примеры:
- `GET /api/users/5` + `permissions:["user:list:read"]` → OK (префикс `user:`).
- `POST /api/courses` + `permissions:["courses:write"]` → OK (точное совпадение).
- Любой запрос + `roles:["admin"]` → OK.

## 4. Проксирование

- Upstream задаётся env: `MODULE_USERS_URL`, `MODULE_COURSES_URL`, `MODULE_QUESTS_URL`,
  `MODULE_TESTS_URL`, `MODULE_ATTEMPTS_URL`, `MODULE_ANSWERS_URL`
  (формат `http://host:port[/prefix]`; quests/attempts/answers по умолчанию
  наследуют `MODULE_TESTS_URL`).
- Шлюз отрезает префикс `/api/<area>`, остаток пути + query уходит вышестоящему модулю.
  `GET /api/users/5?verbose=1` при `MODULE_USERS_URL=http://users:8081` →
  `GET http://users:8081/5?verbose=1`. Базовый path-префикс из URL модуля
  (например `http://users:8081/v1`) подставляется перед остатком.
- Шлюз НЕ пересылает `Authorization`. Вместо этого добавляет:
  `X-Auth-Sub`, `X-Auth-Roles` (через запятую), `X-Auth-Scopes` (через запятую),
  `X-Forwarded-For: <client ip>`.
- Ответ upstream (статус + тело) возвращается клиенту как есть.
- Модуль недоступен → `502 {"error":"bad_gateway"}`; таймаут (`UPSTREAM_TIMEOUT_MS`,
  default 5000) → `504 {"error":"upstream_timeout"}`; upstream не настроен →
  `502 {"error":"bad_gateway","message":"upstream not configured: <area>"}`.

## 5. Конфигурация (env)

| Переменная | Default | Обязат. | Описание |
|------------|---------|---------|----------|
| `BIND` | `0.0.0.0` | нет | Адрес прослушивания |
| `PORT` | `1111` | нет | Порт |
| `JWT_SECRET` | — | **да** | Секрет HS256. Без него шлюз не стартует (fail fast) |
| `JWT_ISSUER` | — | нет | Если задан — проверка `iss` |
| `MODULE_*_URL` | — | нет | Адреса модулей (см. п.4) |
| `UPSTREAM_TIMEOUT_MS` | `5000` | нет | Таймаут запроса к модулю |
| `RECV_TIMEOUT_MS` | `30000` | нет | Таймаут чтения от клиента |
| `MAX_BODY_BYTES` | `1048576` | нет | Лимит тела запроса |
| `MAX_CLIENTS` | `128` | нет | Лимит одновременных соединений |
| `RATE_LIMIT_PER_MIN` | `100` | нет | Лимит запросов с одного IP |
| `LOG_LEVEL` | `info` | нет | `debug`/`info`/`warn`/`error` |

Пример: см. `.env.example`.

## 6. Примеры

```bash
# Health (без токена)
curl http://127.0.0.1:1111/health
# {"status":"ok","version":"1.0.0","upstreams":{"users":"http://127.0.0.1:8081",...}}

# Через шлюз в модуль users
curl http://127.0.0.1:1111/api/users/5 -H "Authorization: Bearer <JWT>"

# Нет токена -> 401
curl -i http://127.0.0.1:1111/api/courses
# HTTP/1.1 401 Unauthorized
# {"error":"unauthorized","message":"missing bearer token"}
```
