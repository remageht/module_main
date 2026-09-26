# INTEGRATION — как подключить свой модуль к шлюзу module_main
# Контракт шлюза: docs/API_CONTRACT.md. Версия: 1.0.0

## 1. Что ожидает шлюз от модуля

Твой модуль — обычный HTTP-сервис (любой язык: C++, Python, Go, ...):

1. Слушает `http://0.0.0.0:<port>` внутри docker-сети (или `127.0.0.1:<port>` локально).
2. Отвечает `GET /health` → `200 {"status":"ok"}` (шлюз это не проверяет на каждый
   запрос, но это нужно для docker healthcheck и мониторинга).
3. Обрабатывает бизнес-пути БЕЗ префикса `/api/<area>`. Шлюз его отрезает:
   `GET /api/users/5` → `GET /5` на `MODULE_USERS_URL` (т.е. модуль описывает
   роуты относительно корня своей area; базовый prefix из URL модуля, например
   `/v1`, подставляется автоматически).
4. Доверяет заголовкам от шлюза (модуль живёт во внутренней сети, напрямую
   из интернета недоступен):
   - `X-Auth-Sub` — id пользователя (`sub` из JWT), может отсутствовать;
   - `X-Auth-Roles` — роли через запятую (`admin,teacher`), может отсутствовать;
   - `X-Auth-Scopes` — права через запятую (`users:read,user:list:read`);
   - `X-Forwarded-For` — IP клиента.
   Заголовок `Authorization` шлюз НЕ пересылает — проверять подпись нужно
   только шлюзу (`JWT_SECRET` живёт только у него).
5. Возвращает статус + тело как есть: шлюз ретранслирует их клиенту без изменений.
   Формат тел — JSON (договорённость команды; шлюз пропустит любой Content-Type).

## 2. Регистрация модуля в шлюзе

1. Выбери area из таблицы (расширение — правкой `apiAreas()` в `src/Router.h`
   + новая env + строка в README/контракт):

   | area | env | default |
   |------|-----|---------|
   | users | `MODULE_USERS_URL` | — |
   | courses | `MODULE_COURSES_URL` | — |
   | quests | `MODULE_QUESTS_URL` | = `MODULE_TESTS_URL` |
   | tests | `MODULE_TESTS_URL` | — |
   | attempts | `MODULE_ATTEMPTS_URL` | = `MODULE_TESTS_URL` |
   | answers | `MODULE_ANSWERS_URL` | = `MODULE_TESTS_URL` |

2. Пропиши URL в `.env` / compose environment, перезапусти шлюз.
   Пустой URL = area отвечает `502 upstream not configured` (шлюз при этом стартует).
3. Проверь: `curl http://<gateway>:1111/health` покажет твой URL в `upstreams`,
   затем запрос с токеном: `curl http://<gateway>:1111/api/<area>/...`.

## 3. Какие JWT выпускать (auth-модуль / разработчик токенов)

- Алгоритм HS256, секрет = `JWT_SECRET` шлюза, обязательно поле `exp`.
- Рекомендуемые claims:
  ```json
  {
    "sub": "user-123",
    "roles": ["teacher"],
    "permissions": ["courses:read", "course:testList"],
    "exp": 1893456000
  }
  ```
- `permissions` могут быть area-wide (`courses:read`, `users:write`) или
  гранулярными из старой схемы (`user:list:read`, `course:test:add`, ...) —
  шлюз понимает оба (см. API_CONTRACT §3.1). Роль `admin` открывает всё.

## 4. docker-compose: добавление модуля

```yaml
services:
  gateway:
    build: .
    ports: ["1111:1111"]
    env_file: [.env]
    environment:
      MODULE_USERS_URL: http://module_users:8081
  module_users:
    image: your-registry/module_users:latest  # твой образ
    expose: ["8081"]
    # healthcheck модуля:
    # healthcheck:
    #   test: ["CMD", "wget", "-qO-", "http://127.0.0.1:8081/health"]
```

## 5. Миграция со старого raw-TCP протокола

Старый режим (сырой `Bearer ...` в TCP без HTTP) удалён. Миграция клиента:
`открыть TCP → послать "Bearer X"` заменить на
`GET /api/<area>/... HTTP/1.1` + `Authorization: Bearer X`.
Коды те же по смыслу, но теперь в JSON-обёртке (см. таблицу ошибок в контракте).
