# 🌱 Система Управления Теплицей UgAgro 95

Полная интегрированная система управления умной теплицей на базе ESP32 с автоматизацией, мониторингом и контролем через n8n, MQTT, PostgreSQL и Telegram.

## 📋 Обзор Проекта

Этот проект представляет собой комплексное решение для автоматизации теплицы, включающее:

- **ESP32 контроллер** - управление датчиками и исполнительными устройствами
- **n8n интеграция** - автоматизация рабочих процессов и обработка данных
- **MQTT** - протокол связи между компонентами
- **PostgreSQL** - хранение данных датчиков и истории
- **Telegram бот** - мониторинг и управление через мессенджер
- **Веб-дашборд** - визуализация данных в реальном времени

## 🏗️ Архитектура Системы

```
┌─────────────────────────────────────────────────────────┐
│                    ESP32 UgAgro_95                       │
│  (Датчики + Actuators + MQTT Client)                    │
└──────────────────┬──────────────────────────────────────┘
                   │ MQTT (broker.emqx.io)
                   │ Topic: greenhouse/esp32/all_sensors
                   ↓
┌─────────────────────────────────────────────────────────┐
│              n8n Automation Platform                     │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Workflow: MQTT Data Pipeline                     │  │
│  │  → Парсинг данных → PostgreSQL                   │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Workflow: Telegram Critical Alerts               │  │
│  │  → Мониторинг (20 сек) → Оповещения              │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Workflow: Telegram Commands (/status)            │  │
│  │  → Обработка команд → Отправка статуса           │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Workflow: Telegram Callback Handler              │  │
│  │  → Обработка кнопок → State management            │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Workflow: Data Rotation Cleanup                  │  │
│  │  → Ежедневная очистка старых данных              │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  Workflow: Web Dashboard                          │  │
│  │  → Визуализация → Chart.js графики               │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                   │
                   ↓
┌─────────────────────────────────────────────────────────┐
│              PostgreSQL Database                         │
│  • ugagro_readings (показания датчиков)                 │
│  • telegram_alert_states (состояния оповещений)         │
│  • ugagro_alerts_history (история оповещений)           │
└─────────────────────────────────────────────────────────┘
                   │
                   ↓
┌─────────────────────────────────────────────────────────┐
│         Пользовательские Интерфейсы                     │
│  • Telegram Bot (команды и оповещения)                  │
│  • Web Dashboard (графики и мониторинг)                 │
└─────────────────────────────────────────────────────────┘
```

## 📁 Структура Проекта

```
Balagan/
├── PROJECT_README.md                      # Этот файл (общее описание)
│
├── UgAgro_95_refactored/                  # ESP32 контроллер теплицы
│   ├── README.md                          # Документация ESP32
│   ├── PROGRESS.md                        # Прогресс разработки
│   ├── UgAgro_95_refactored.ino          # Главный файл прошивки
│   ├── Config.h                           # Конфигурация (WiFi, MQTT, пины)
│   ├── Sensors.h/cpp                      # Модуль датчиков (Modbus)
│   ├── Actuators.h/cpp                    # Модуль исполнительных устройств
│   ├── MqttManager.h/cpp                  # MQTT клиент
│   ├── Storage.h/cpp                      # Хранилище настроек (LittleFS)
│   ├── ControlLogic.h/cpp                 # Логика управления теплицей
│   ├── commands/                          # Command Pattern для настроек
│   │   ├── ICommand.h
│   │   ├── CommandDispatcher.h
│   │   ├── SettingCommands.h
│   │   └── AllCommands.h
│   └── data/
│       └── index.html                     # Веб-интерфейс ESP32
│
└── n8n-greenhouse-integration/            # n8n автоматизация
    ├── README.md                          # Документация n8n
    ├── database/
    │   └── init-greenhouse-db.sql         # Схема PostgreSQL
    ├── docs/
    │   ├── BEGET_VPS_DEPLOYMENT.md       # Развертывание на VPS
    │   ├── MQTT_DATA_STRUCTURE.md        # Структура MQTT данных
    │   ├── POSTGRES_SETUP.md             # Настройка PostgreSQL
    │   └── PUTTY_SETUP.md                # Настройка через SSH
    └── workflows/
        ├── 01-mqtt-data-pipeline.json    # Прием данных от ESP32
        ├── 05-data-rotation-cleanup.json # Очистка старых данных
        ├── 06-web-dashboard-simple.json  # Веб-дашборд
        └── telegram-unified.json          # Telegram бот (команды, оповещения, callbacks)
```

## 🚀 Быстрый Старт

### 1. Настройка ESP32 (UgAgro_95_refactored)

**Требования:**
- ESP32 Dev Module (4MB Flash)
- Arduino IDE с библиотеками:
  - ArduinoJson v7.x
  - AsyncTCP, ESPAsyncWebServer
  - PubSubClient (MQTT)
  - ModbusMaster, RTClib

**Шаги:**
1. Откройте `UgAgro_95_refactored/UgAgro_95_refactored.ino` в Arduino IDE
2. Настройте параметры в `Config.h`:
   - WiFi SSID и пароль
   - MQTT брокер (по умолчанию `broker.emqx.io`)
3. Загрузите прошивку на ESP32
4. Проверьте Serial Monitor (115200 baud)

**Подробнее:** [UgAgro_95_refactored/README.md](UgAgro_95_refactored/README.md)

### 2. Настройка PostgreSQL

**Шаги:**
1. Запустите PostgreSQL (Docker или локально)
2. Выполните SQL скрипт:
   ```bash
   psql -U root -d n8n < n8n-greenhouse-integration/database/init-greenhouse-db.sql
   ```
3. Проверьте создание таблиц: `ugagro_readings`, `telegram_alert_states`, `ugagro_alerts_history`

**Подробнее:** [n8n-greenhouse-integration/docs/POSTGRES_SETUP.md](n8n-greenhouse-integration/docs/POSTGRES_SETUP.md)

### 3. Настройка n8n Workflows

**Требования:**
- n8n v1.115.3+ (self-hosted)
- Telegram Bot Token (через @BotFather)
- Telegram Chat ID (через @userinfobot)

**Шаги:**
1. Создайте credentials в n8n:
   - PostgreSQL (host: `postgres`, database: `n8n`)
   - MQTT (host: `broker.emqx.io`, port: `1883`)
   - Telegram API (token от @BotFather)
2. Импортируйте workflows из `n8n-greenhouse-integration/workflows/`
3. Настройте Telegram Chat ID в workflows
4. Активируйте все workflows

**Подробнее:** [n8n-greenhouse-integration/README.md](n8n-greenhouse-integration/README.md)

## ✨ Основные Функции

### ESP32 Контроллер

**Датчики (Modbus RS485):**
- 🌡️ Температура и влажность (2 датчика)
- 💨 Датчик ветра
- ☀️ Пиранометр (солнечная радиация)
- 🌱 Мат-сенсор (температура, влажность, EC, pH субстрата)
- 💧 Датчик уровня раствора
- 🌦️ Наружный датчик (температура, влажность)

**Исполнительные Устройства (10 реле):**
- 🪟 Окна (плавное управление 0-100%)
- 🔥 Отопление (с гистерезисом)
- 🌀 Вентилятор (с интервалами)
- 💦 Туманообразование (клапан + насос)
- 🚿 Полив (5 режимов: утренний, авто, периодический, forced, override)
- 🌡️ Нагрев раствора

**Логика Управления:**
- Автоматический режим на основе датчиков
- Ручное управление через MQTT/Telegram
- Восстановление состояния после перезагрузки
- FreeRTOS задачи для параллельной обработки

### n8n Автоматизация

**MQTT Data Pipeline:**
- Прием данных от ESP32 каждые 5 секунд
- Сохранение в PostgreSQL (30+ параметров)
- Обработка конфликтов (ON CONFLICT DO UPDATE)

**Telegram Bot:**
- `/status` - текущие показания теплицы с цветовой индикацией
- Критические оповещения каждые 20 секунд
- Inline кнопки (✅ Подтвердить, 🔕 Отключить на 1ч)
- State management (предотвращение спама)

**Веб-дашборд:**
- Графики Chart.js (последние 6 часов)
- Карточки с текущими показаниями
- Автообновление каждые 30 секунд
- Адаптивный дизайн

**Data Rotation:**
- Ежедневная очистка в 02:00
- Удаление показаний старше 7 дней
- Удаление истории оповещений старше 30 дней

## 🔧 Конфигурация

### ESP32 Config.h

```cpp
// WiFi
const char* const WIFI_SSID = "your_wifi_ssid";
const char* const WIFI_PASS = "your_wifi_password";

// MQTT
const char* const MQTT_BROKER = "broker.emqx.io";
const int MQTT_PORT = 1883;
const char* const MQTT_TOPIC_BASE = "greenhouse";

// NTP
const char* const NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET = 10800;  // UTC+3 (Москва)
```

### n8n Credentials

**PostgreSQL:**
- Host: `postgres` (Docker) или `localhost`
- Database: `n8n`
- User: `root`
- Password: `your_password`

**MQTT:**
- Protocol: `mqtt`
- Host: `broker.emqx.io`
- Port: `1883`

**Telegram:**
- Access Token: `от @BotFather`
- Chat ID: `от @userinfobot`

## 📊 Мониторинг

### Критические Пороги

| Параметр | Норма | Предупреждение | Критично |
|----------|-------|----------------|----------|
| Температура | 10-40°C | < 10°C или > 40°C | - |
| Влажность | 20-85% | < 20% или > 85% | - |
| Уровень воды | > 5% | < 10% | < 5% |
| Скорость ветра | < 11 м/с | > 11 м/с | > 15 м/с |

### Telegram Команды

**`/status`** или **`/статус`**
- Отображает текущие показания
- Цветовая индикация: 🟢 норма, 🟡 предупреждение, 🔴 критично
- Состояния всех устройств

**Автоматические оповещения:**
- 🔥 Высокая температура (> 40°C)
- ❄️ Низкая температура (< 10°C)
- 💧 Высокая влажность (> 85%)
- 🏜️ Низкая влажность (< 20%)
- 🚨 Низкий уровень воды (< 5%)
- 💨 Сильный ветер (> 11 м/с)

## 🔄 MQTT Топики

### Публикация от ESP32 (каждые 5 сек)

- `greenhouse/esp32/all_sensors` - все данные в одном JSON
- `greenhouse/sensors/temperature` - температура
- `greenhouse/sensors/humidity` - влажность
- `greenhouse/sensors/level` - уровень воды
- `greenhouse/actuators/window` - позиция окна
- `greenhouse/actuators/equipment` - состояния устройств

### Подписка ESP32 (команды)

- `greenhouse/set/equipment` - управление оборудованием
- `greenhouse/set/window` - управление окнами
- `greenhouse/set/hydro` - гидропоника
- `greenhouse/set/settings` - настройки

## 🗄️ База Данных

### Таблицы PostgreSQL

**ugagro_readings** (7 дней ретенция)
- Показания всех датчиков
- Состояния всех устройств
- Режимы полива и статистика

**telegram_alert_states**
- Текущие состояния оповещений
- Подтверждения и mute timestamps
- Счетчики отправок

**ugagro_alerts_history** (30 дней ретенция)
- История всех оповещений
- Уровень severity
- Значения сенсоров при оповещении

## 📈 Производительность

**ESP32:**
- Размер прошивки: ~700 KB (из 1.3 MB)
- Использование RAM: ~180 KB (из 327 KB)
- Частота опроса датчиков: 1 сек
- Частота MQTT публикации: 5 сек

**n8n:**
- MQTT Pipeline: реал-тайм (на событие)
- Telegram Alerts: каждые 20 секунд
- Data Rotation: ежедневно 02:00

**PostgreSQL:**
- Ретенция показаний: 7 дней
- Ретенция оповещений: 30 дней
- Автоматическая очистка

## 🛠️ Устранение Неполадок

### ESP32 не подключается к WiFi
1. Проверьте SSID и пароль в `Config.h`
2. Проверьте Serial Monitor (115200 baud)
3. Убедитесь что WiFi 2.4 GHz (ESP32 не поддерживает 5 GHz)

### MQTT данные не приходят в n8n
1. Проверьте что ESP32 подключен к MQTT (Serial Monitor)
2. Проверьте топик в n8n workflow: `greenhouse/esp32/all_sensors`
3. Проверьте что workflow активен (зеленый переключатель)

### Telegram бот не отвечает
1. Напишите боту `/start`
2. Проверьте TOKEN и CHAT_ID
3. Проверьте что workflow активен

### PostgreSQL ошибки
1. Проверьте что контейнер запущен: `docker ps`
2. Проверьте credentials (host, user, password)
3. Проверьте что таблицы созданы: `\dt` в psql

## 📚 Документация

- **[UgAgro_95_refactored/README.md](UgAgro_95_refactored/README.md)** - Подробная документация ESP32
- **[n8n-greenhouse-integration/README.md](n8n-greenhouse-integration/README.md)** - Подробная документация n8n
- **[POSTGRES_SETUP.md](n8n-greenhouse-integration/docs/POSTGRES_SETUP.md)** - Настройка PostgreSQL
- **[BEGET_VPS_DEPLOYMENT.md](n8n-greenhouse-integration/docs/BEGET_VPS_DEPLOYMENT.md)** - Развертывание на VPS

## 🔮 Планы Развития

- [ ] Добавить графики исторических данных (недели, месяцы)
- [ ] Машинное обучение для предсказания оптимальных режимов
- [ ] Интеграция с погодными API
- [ ] Мобильное приложение (iOS/Android)
- [ ] Голосовое управление (Alexa, Google Home)
- [ ] Поддержка нескольких теплиц
- [ ] Система камер для визуального мониторинга

## 📄 Лицензия

MIT License

## 👨‍💻 Разработчики

**ESP32 Firmware:** Рефакторинг с помощью Claude (Anthropic AI)
**n8n Integration:** Интеграция и автоматизация
**Telegram Bot:** Мониторинг и управление

## 🤝 Вклад

Pull requests приветствуются! Для крупных изменений сначала откройте issue.

## 📞 Поддержка

Если возникли вопросы или проблемы:
1. Проверьте раздел [Устранение Неполадок](#устранение-неполадок)
2. Откройте issue в GitHub
3. Проверьте логи:
   - ESP32: Serial Monitor (115200 baud)
   - n8n: `docker logs n8n`
   - PostgreSQL: `docker logs postgres`

---

**Версия проекта:** 2.4-beta
**Последнее обновление:** 2025-11-20

🌱 Автоматизация теплиц будущего уже сегодня!
