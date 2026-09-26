# module_main — API-шлюз (gateway v1)

### Реализация Главного модуля по дисциплине "Алгоритмизация и программирование"

<br>
<p align="right">Выполнил: Ваджипов Эмир Аленович</p>
<p align="right">Группа: ПИ-б-о-241(1)</p>
<p align="right">Преподаватель: Чабанов Владимир Викторович</p>

## Что это

Единая точка входа для всех клиентов: проверяет JWT (HS256), определяет права
по claims (`permissions` / `roles`) и проксирует `GET|POST|PUT|PATCH|DELETE
/api/<area>/...` в соответствующий модуль (`users`, `courses`, `quests`,
`tests`, `attempts`, `answers`). Контракт: [docs/API_CONTRACT.md](docs/API_CONTRACT.md),
подключение модулей: [docs/INTEGRATION.md](docs/INTEGRATION.md).

Кроссплатформенно: Windows (Winsock) + Linux/macOS (BSD sockets), C++17, CMake.

## Быстрый старт (локально)

```bash
cp .env.example .env   # Windows: copy .env.example .env
# ... вписать JWT_SECRET и MODULE_*_URL ...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/main_module            # Windows: .\build\Release\main_module.exe
curl http://127.0.0.1:1111/health
```

Требования: CMake ≥ 3.16, компилятор C++17, OpenSSL
(на Windows используются завендоренные `src/lib`, на Linux — системный).

## Docker

```bash
cp .env.example .env   # заполнить JWT_SECRET!
docker compose up --build -d
docker compose ps
curl http://127.0.0.1:1111/health
docker compose logs --tail=100 gateway
```

## Проверки

```powershell
powershell -ExecutionPolicy Bypass -File tests/test_static_checks.ps1
```

CI (GitHub Actions): сборка Windows + Linux, статические проверки, сборка образа.
