# Детальный анализ кода UgAgro_95.ino

**Дата анализа:** 2025-11-16  
**Размер файла:** 4164 строки  
**Платформа:** ESP32 (Arduino)

---

## 1. СТРУКТУРА ФУНКЦИЙ И ИХ РАСПРЕДЕЛЕНИЕ

### 1.1 Полный список функций (27 функций)

| # | Функция | Строка | Размер (строк) | Назначение |
|---|---------|--------|--------|-----------|
| 1 | `mapFloat()` | 219 | 3 | Линейное масштабирование значений |
| 2 | `isNight()` | 223 | 4 | Проверка ночного времени |
| 3 | `calcFogDuration()` | 228 | 4 | Расчет длительности туманообразования |
| 4 | `calcFogInterval()` | 233 | 4 | Расчет интервала туманообразования |
| 5 | `publishAllSensors()` | 238 | 50 | Публикация всех датчиков в MQTT |
| 6 | `addToHistory()` | 288 | 28 | Добавление записи в историю |
| 7 | `applyDefaultSettings()` | 316 | 105 | Применение настроек по умолчанию |
| 8 | `isSettingsZeroed()` | 421 | 7 | Проверка обнулены ли настройки |
| 9 | `loadSettings()` | 428 | 159 | Загрузка настроек из памяти |
| 10 | `publishSettings()` | 587 | 106 | Публикация настроек в MQTT |
| 11 | `saveSettings()` | 693 | 127 | Сохранение настроек в LittleFS |
| 12 | `publishHistory()` | 820 | 28 | Публикация истории в MQTT |
| 13 | `saveHistory()` | 848 | 35 | Сохранение истории в LittleFS |
| 14 | `loadHistory()` | 883 | 41 | Загрузка истории из памяти |
| 15 | `resetForcedToNormal()` | 924 | 41 | Переход из forced режима в обычный |
| 16 | `callback()` (MQTT) | 965 | **822** | Обработчик MQTT сообщений |
| 17 | `startWindowMovement()` | 1787 | 15 | Старт движения окна |
| 18 | `stopWindowMovement()` | 1802 | 6 | Остановка движения окна |
| 19 | `updateWindowPosition()` | 1808 | 17 | Обновление позиции окна |
| 20 | `forceUpdateAndValidateTime()` | 1825 | 25 | Принудительное обновление времени |
| 21 | `networkTimeManagementTask()` | 1850 | 128 | FreeRTOS задача управления сетью/временем |
| 22 | `readFloatModMaster()` | 1972 | 21 | Чтение Modbus данных (float) |
| 23 | `modbusTask()` | 1993 | 78 | FreeRTOS задача Modbus |
| 24 | `logicTask()` | 2071 | **919** | FreeRTOS основная логика (ОСНОВНАЯ) |
| 25 | `setup()` | 2990 | 1174 | Инициализация системы |
| 26 | `loop()` | 4165 | 1 | Пуста (все в FreeRTOS) |
| 27 | HTTP Handlers (inline) | 3126-4160 | ~1000 | Web API обработчики |

**Итого: 27 функций (плюс ~5 inline лямбда-функций в setup)**

---

## 2. ОСНОВНЫЕ МОДУЛИ И ОБЛАСТИ ФУНКЦИОНАЛЬНОСТИ

### 2.1 Архитектура по функциональным блокам

```
┌─────────────────────────────────────────────────────────────┐
│                    УМНАЯ ТЕПЛИЦА UgAgro                    │
├─────────────────────────────────────────────────────────────┤
│
├─ ДАТЧИКИ И ВВОД-ВЫВОД (I²C/Modbus/GPIO)
│  ├─ RTC DS3231 (реальное время)
│  ├─ Modbus RTU (7 модулей):
│  │  ├─ SLAVE_TEMP1/2 (температура/влажность)
│  │  ├─ SLAVE_WIND (ветер)
│  │  ├─ SLAVE_PYRANO (радиация)
│  │  ├─ SLAVE_MAT_SENSOR (влажность мата)
│  │  ├─ SLAVE_SOL (раствор)
│  │  ├─ SLAVE_LEVEL (уровень воды)
│  │  └─ SLAVE_OUTDOOR (уличные условия)
│  ├─ GPIO реле (10 выходов):
│  │  ├─ REL_WINDOW_UP/DN (окна)
│  │  ├─ REL_HEAT (отопление)
│  │  ├─ REL_FAN (вентилятор)
│  │  ├─ REL_FOG_VALVE/PUMP (туман)
│  │  ├─ REL_PUMP (полив)
│  │  ├─ REL_FILL (заполнение)
│  │  ├─ REL_3WAY (3-ходовой клапан)
│  │  └─ REL_SOL (нагрев раствора)
│
├─ СЕТЕВЫЕ И КОММУНИКАЦИОННЫЕ
│  ├─ WiFi (2.4ГГц) для подключения к MQTT
│  ├─ NTP (синхронизация времени)
│  ├─ MQTT (EMQX broker) публикация/подписка
│  │  ├─ greenhouse/esp32/all_sensors
│  │  ├─ greenhouse/esp32/settings_full
│  │  ├─ greenhouse/esp32/history
│  │  ├─ greenhouse/esp32/window
│  │  ├─ greenhouse/esp32/status
│  │  └─ greenhouse/set_settings (подписка)
│  │  └─ greenhouse/cmd/# (подписка)
│  └─ HTTP Web Server (AsyncWebServer, порт 80)
│     ├─ GET  /                (HTML UI)
│     ├─ GET  /settings        (JSON)
│     ├─ POST /settings        (JSON)
│     ├─ POST /cmd/watering    (управление поливом)
│     ├─ POST /cmd/window      (управление окнами)
│     ├─ POST /cmd/equipment   (управление оборудованием)
│     ├─ GET  /data            (текущие данные)
│     └─ GET  /history         (история)
│
├─ УПРАВЛЕНИЕ ОКНАМИ (climate control)
│  ├─ startWindowMovement() - инициация движения
│  ├─ updateWindowPosition() - обновление позиции
│  ├─ stopWindowMovement() - остановка
│  └─ Логика: положение 0-100%, время открытия/закрытия
│
├─ УПРАВЛЕНИЕ ОТОПЛЕНИЕМ
│  ├─ Пороги: heatOn/heatOff (температура)
│  ├─ Ручной режим: manualHeat
│  ├─ Гистерезис: 0.2°C
│  └─ Интервал изменения: 5000мс
│
├─ УПРАВЛЕНИЕ ВЕНТИЛЯЦИЕЙ
│  ├─ Интервал включения: fanInterval
│  ├─ Длительность: fanDuration
│  ├─ Интервал изменения: 5000мс
│  └─ Ручной режим: manualFan
│
├─ УПРАВЛЕНИЕ ТУМАНООБРАЗОВАНИЕМ (fogging)
│  ├─ 3 режима: Auto(0), Manual(1), Forced(2)
│  ├─ Временные окна:
│  │  ├─ Morning: fogMorningStart/End
│  │  └─ Day: fogDayStart/End
│  ├─ Расчет длительности и интервала на основе:
│  │  ├─ Влажности (fogMinHum/MaxHum)
│  │  └─ Температуры (fogMinTemp/MaxTemp)
│  ├─ Задержка fogDelay перед включением насоса
│  └─ Принудительное включение: forceFogOn
│
├─ УПРАВЛЕНИЕ ПОЛИВОМ (irrigation)
│  ├─ 3 режима водинга:
│  │  ├─ 0: Автоматический (на основе радиации)
│  │  ├─ 1: Ручной (на основе расписания)
│  │  └─ 2: Принудительный (forced)
│  ├─ Автоматический полив:
│  │  ├─ Накопление радиации (radSum)
│  │  ├─ Пороговое значение (radThreshold)
│  │  ├─ Интервал проверки (radCheckInterval)
│  │  └─ Длительность полива (pumpTime)
│  ├─ Ручной полив:
│  │  ├─ Расписание (4 режима с часами)
│  │  ├─ Минимальный интервал (minWateringInterval)
│  │  ├─ Максимальный интервал (maxWateringInterval)
│  │  └─ Длительность (manualWateringDuration)
│  ├─ Утренний полив (morning):
│  │  ├─ Количество циклов (morningWateringCount)
│  │  ├─ Интервал между циклами (morningWateringInterval)
│  │  └─ Длительность цикла (morningWateringDuration)
│  ├─ Заполнение бака (fill):
│  │  └─ Максимум 2 часа, контроль уровня
│  ├─ Гидромикс (hydro-mix):
│  │  ├─ Длительность (hydroMixDuration)
│  │  └─ Задержка перед запуском (hydroStart)
│  └─ Ручное управление: manualPump, manualPumpOverride
│
├─ НАГРЕВ РАСТВОРА (solution heating)
│  ├─ Пороги: solOn/solOff
│  ├─ Ручной режим: manualSol
│  └─ Условие: level > 20%
│
├─ КОНТРОЛЬ УРОВНЯ ВОДЫ
│  ├─ Минимум/максимум (levelMin/Max)
│  └─ Интервал проверки для заполнения
│
├─ КОНТРОЛЬ ВЕТРА
│  ├─ Пороги: wind1, wind2
│  ├─ Блокировка окон при ветре
│  ├─ Длительность блокировки (windLockMinutes)
│  └─ Проверка в logicTask()
│
├─ КОНТРОЛЬ ВРЕМЕНИ И ЦИКЛОВ
│  ├─ Day/Night определение (dayStart/nightStart)
│  ├─ Сброс счётчиков по дням
│  ├─ Максимум циклов полива (maxWateringCycles)
│  └─ Таймеры последних действий
│
├─ ХРАНЕНИЕ И ИСТОРИЯ
│  ├─ Настройки: LittleFS (/settings.json, ~200 полей)
│  ├─ История: LittleFS (/history.json, 20 записей)
│  ├─ Cooldown сохранения: 5000мс
│  └─ Сохранение с лог-вызывающем функции
│
├─ МНОГОПОТОЧНОСТЬ (FreeRTOS на ESP32)
│  ├─ logicTask() на Core 0 (919 строк)
│  ├─ modbusTask() (78 строк)
│  ├─ networkTimeManagementTask() (128 строк)
│  ├─ HTTP обработчики в основном потоке
│  ├─ Semaphore: mtx для защиты общих данных
│  ├─ Queues: qAvg, qSol, qLevel, qWind, qOutdoor, qPyrano, qMat
│  └─ vTaskDelay() для синхронизации
│
└─ КОНФИГУРАЦИЯ И ОТЛАДКА
   ├─ DEBUG флаг (0 = disabled)
   ├─ Serial логирование (115200 baud)
   ├─ JSON документы для конфигурации
   └─ Сохранение параметров вызова функции (caller)
```

---

## 3. ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ И СТРУКТУРЫ ДАННЫХ

### 3.1 Основная структура Settings (199 полей)

```cpp
struct Settings {  // Строки 100-199
  // КЛИМАТ
  float minTemp, maxTemp;                // Диапазон температуры
  float minHum, maxHum;                  // Диапазон влажности
  uint32_t openTime, closeTime;          // Время открытия/закрытия окна (мс)
  
  // ОТОПЛЕНИЕ И ОХЛАЖДЕНИЕ
  float heatOn, heatOff;                 // Пороги включения/отключения
  float solOn, solOff;                   // Пороги для нагрева раствора
  
  // ВЕНТИЛЯЦИЯ
  uint32_t fanInterval;                  // Интервал включения
  uint32_t fanDuration;                  // Длительность работы
  
  // ТУМАНООБРАЗОВАНИЕ (19 параметров)
  uint8_t fogMorningStart, fogMorningEnd;
  uint8_t fogDayStart, fogDayEnd;
  uint32_t fogMorningDuration, fogMorningInterval;
  uint32_t fogDayDuration, fogDayInterval;
  uint32_t fogDelay;
  float fogMinHum, fogMaxHum;
  float fogMinTemp, fogMaxTemp;
  uint32_t fogMinDuration, fogMaxDuration;
  uint32_t fogMinInterval, fogMaxInterval;
  int fogMode;                           // 0=Auto, 1=Manual, 2=Forced
  bool forceFogOn;
  uint32_t autoFogDayStart, autoFogDayEnd;
  
  // ПОЛИВ (28 параметров)
  WateringMode wateringModes[4];         // 4 расписания
  int wateringMode;                      // 0=Auto, 1=Manual, 2=Forced
  uint32_t pumpTime;                     // Длительность полива (мс)
  uint8_t wateringStartHour, StartMinute, EndHour, EndMinute;
  uint32_t minWateringInterval;
  uint32_t maxWateringInterval;
  uint32_t manualWateringInterval;
  uint32_t manualWateringDuration;
  uint32_t radThreshold;                 // Порог радиации
  uint32_t radSumResetInterval;
  uint32_t radCheckInterval;
  float radSum;                          // Накопленная радиация
  uint8_t maxWateringCycles;
  uint8_t morningWateringCount;
  uint32_t morningWateringInterval;
  uint32_t morningWateringDuration;
  uint32_t forcedWateringDuration;
  uint32_t hydroMixDuration;
  float matMinHumidity, matMaxEC;
  
  // СОСТОЯНИЯ ОБОРУДОВАНИЯ
  bool heatState, fanState, fogState;
  bool pumpState, solHeatState;
  bool hydroMix, fillState;
  bool manualHeat, manualPump, manualSol;
  bool manualFan, manualWindow;
  
  // ВЕТЕР И ВРЕМЯ
  float wind1, wind2;
  uint32_t windLockMinutes;
  uint8_t nightStart, dayStart;
  float nightOffset;
  
  // УРОВЕНЬ ВОДЫ
  float levelMin, levelMax;
  
  // СОСТОЯНИЯ ПРОЦЕССОВ (многие для восстановления после перезагрузки)
  bool forceWateringActive;
  bool forceWateringOverride;
  uint32_t forceWateringEndTimeUnix;
  uint32_t forceWateringEndTime;
  int prevWateringMode;
  bool forcedWateringPerformed;           // Флаг реального полива в forced режиме
  
  // СОХРАНЕНИЕ СОСТОЯНИЯ УТРЕННЕГО ПОЛИВА
  bool morningStartedToday;
  bool morningWateringActive;
  uint8_t currentMorningWatering;
  uint32_t lastMorningWateringStartUnix;
  uint32_t lastMorningWateringEndUnix;
  bool pendingMorningComplete;
  
  // ТАЙМЕРЫ И СИНХРОНИЗАЦИЯ
  uint32_t lastWateringStartUnix;
  uint32_t lastSunTimeUnix;
  uint32_t lastManualWateringTimeUnix;
  uint32_t hydroStartUnix;
  uint32_t cycleCount;                   // Счётчик циклов полива
};
```

**Итого: 199 полей! Крайне большая структура.**

### 3.2 Вспомогательные структуры

```cpp
struct HistoryEntry {                   // Строки 63-79 (20 записей)
  uint32_t timestamp;
  float avgTemp, avgHum;
  float level, windSpeed, outdoorTemp, outdoorHum;
  float windowPosition;
  bool fanState, heatingState, wateringState;
  bool solutionHeatingState, hydroState, fillState, fogState;
};

struct WateringMode {                   // Строки 94-99
  bool enabled;
  uint8_t startHour, endHour;
  uint32_t duration;
};

typedef struct {                        // Строки 200-203
  float temperature, humidity;
} SensorData;

typedef struct {                        // Строки 204-209
  float temperature, humidity, ec, ph;
} MatSensorData;
```

### 3.3 Глобальные переменные (non-struct)

```cpp
Settings sett;                          // Основная структура
HistoryEntry history[HISTORY_SIZE];     // История (20 записей)
int historyIndex = 0;

// УПРАВЛЕНИЕ ОКНАМИ
int windowPos = 0;
bool windowMoving = false;
String windowDirection = "";
uint32_t windowStartTime = 0;
int targetWindowPos = 0;
uint32_t moveDuration = 0;

// ТАЙМЕРЫ
unsigned long lastSaveTime = 0;
const unsigned long saveCooldown = 5000;

// СЕТЕВЫЕ
RTC_DS3231 rtc;
HardwareSerial RS485(1);
ModbusMaster mb;
WiFiClient net;
PubSubClient client(net);
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 10800);
AsyncWebServer server(80);

// ОЧЕРЕДИ И СЕМАФОРЫ (FreeRTOS)
QueueHandle_t qAvg, qSol, qLevel, qWind, qOutdoor;
QueueHandle_t qPyrano, qMat;
SemaphoreHandle_t mtx;

// ЗАДАЧИ
TaskHandle_t networkTimeTaskHandle = NULL;
volatile bool forceWifiReconnect = false;
volatile bool wifiConnectedOnce = false;
```

---

## 4. ЗАВИСИМОСТИ МЕЖДУ КОМПОНЕНТАМИ

### 4.1 Диаграмма потока данных

```
ДАТЧИКИ (Modbus RTU)
    ↓
    ├→ modbusTask()
         ├→ qAvg (температура/влажность)
         ├→ qSol
         ├→ qLevel
         ├→ qWind
         ├→ qOutdoor
         ├→ qPyrano
         └→ qMat
    ↓
logicTask()  ← ОСНОВНАЯ ЛОГИКА
    ├→ Читает очереди
    ├→ Применяет правила управления
    ├→ Устанавливает состояния GPIO (digitalWrite)
    ├→ Вычисляет время полива/туманирования
    ├→ Управляет окнами
    ├→ Проверяет пороги
    └→ Вызывает publishAllSensors()
         └→ client.publish() (MQTT)
    ↓
networkTimeManagementTask()
    ├→ WiFi подключение
    ├→ NTP синхронизация
    └→ MQTT retry logic
    ↓
callback() (MQTT)
    ├→ Получает команды
    ├→ Обновляет sett
    ├→ saveSettings()
    └→ publishSettings()
    ↓
HTTP обработчики в setup()
    ├→ GET /settings → publishAllSensors()
    ├→ POST /settings → saveSettings()
    ├→ POST /cmd/watering → управление поливом
    ├→ POST /cmd/window → управление окнами
    └→ POST /cmd/equipment → управление оборудованием
    ↓
Память (LittleFS)
    ├→ /settings.json (загруженность ~200 строк)
    └→ /history.json (загруженность 20 записей)
```

### 4.2 Критические зависимости

1. **logicTask ← modbusTask**: логика полива зависит от показаний датчиков
2. **logicTask ← settings**: все правила основаны на параметрах в sett
3. **callback ← MQTT**: команды из облака обновляют sett
4. **HTTP handlers ← sett**: все эндпоинты работают с настройками
5. **WiFi/Time ← networkTimeTask**: точное время критично для расписаний
6. **GPIO outputs ← logicTask**: состояния реле управляются только из logicTask

---

## 5. ДУБЛИРОВАНИЕ КОДА И ПРОБЛЕМЫ

### 5.1 Критическое дублирование в callback() и HTTP POST /settings

**Проблема**: Код парсинга JSON полей дублируется ДВАЖДЫ (в callback() и в POST /settings)
- **Строки 987-1680**: callback() обработка 93 полей
- **Строки 3250-3850**: POST /settings обработка тех же 93 полей

**Пример дублирования** (минТемпература):

```cpp
// callback() - строки 989-995
if (doc.containsKey("minTemp")) {
  float val = doc["minTemp"];
  if (sett.minTemp != val) {
    sett.minTemp = val;
    needSave = true;
  }
}

// POST /settings - строки 3250-3256 (ИДЕНТИЧНО!)
if (doc.containsKey("minTemp")) {
  float val = doc["minTemp"];
  if (sett.minTemp != val) {
    sett.minTemp = val;
    needSave = true;
  }
}
```

**Масштаб проблемы**: 93 поля × 2 места = 558 строк дублированного кода!

**Рекомендация**: Создать вспомогательную функцию:
```cpp
bool updateSetting(const char* key, DynamicJsonDocument& doc, /*типы*/) {
  // Единая логика для всех полей
}
```

### 5.2 Дублирование в MQTT публикации

**Проблема**: publishSettings() вручную копирует все поля
- Строки 587-690 (106 строк кода для копирования 60+ полей)
- Нет сериализации структуры, всё вручную

**Пример**:
```cpp
doc["minTemp"] = sett.minTemp;
doc["maxTemp"] = sett.maxTemp;
doc["minHum"] = sett.minHum;
// ... 60+ таких строк
```

**Рекомендация**: Использовать автоматическую сериализацию через ArduinoJson

### 5.3 Повторяющиеся проверки условий

**Проблема**: Проверка времени дня/ночи повторяется в разных местах

```cpp
// logicTask() строка 2111+
bool morning = (h >= fogMorningStart && h < fogMorningEnd);
bool day = (h >= fogDayStart && h < fogDayEnd);

// Повторяется несколько раз в разных блоках логики
```

### 5.4 Множественные проверки состояния manualX

```cpp
// Проверки manualHeat/manualPump/manualFan повторяются:
// - в callback() (MQTT)
// - в POST /cmd/equipment (HTTP)
// - в logicTask() (основная логика)
```

---

## 6. ОЦЕНКА СЛОЖНОСТИ ФУНКЦИЙ

### 6.1 Top-5 самых сложных функций

| Ранг | Функция | Строк | Сложность | Проблемы |
|------|---------|-------|-----------|----------|
| 1 | **logicTask()** | 919 | КРИТИЧЕСКАЯ | Монолитная, 35+ static переменных, вложенность 5 уровней, 223+ условия |
| 2 | **callback()** (MQTT) | 822 | КРИТИЧЕСКАЯ | 93 поля обновления, высокая нложенность, повторяющиеся блоки |
| 3 | **setup()** | 1174 | ВЫСОКАЯ | Инициализация всех подсистем, 8 server.on(), создание задач |
| 4 | **saveSettings()** | 127 | СРЕДНЯЯ | Много JSON операций, циклы для массивов, обработка ошибок |
| 5 | **loadSettings()** | 159 | СРЕДНЯЯ | Парсинг большого JSON (60+ полей) |

### 6.2 Анализ logicTask() (919 строк - МОНОЛИТ)

**Статические переменные внутри logicTask():**
```cpp
static bool heat = false;                           // 35 static переменных!
static bool fan = false;
static bool fog = false;
static bool pump = false;
static bool solHeat = false;
static uint32_t windLockStart = 0;
static bool windRestricted = false;
static uint32_t lastHistory = 0;
static uint32_t lastHour = 0;
static uint32_t lastWater = 0;
static uint32_t taskFogStartTime = 0;
static uint32_t taskLastFog = 0;
static bool fogValveOn = false;
static uint32_t fogValveStartTime = 0;
// ... и ещё 22 переменные
```

**Структура logicTask()**:
```
logicTask() {
  // Инициализация (79 строк)
  // Бесконечный цикл:
  └─ for(;;) {
     ├─ Читание из очередей (20 строк)
     ├─ Инициализация после перезагрузки (50 строк)
     ├─ УТРЕННИЙ ПОЛИВ (75 строк) - вложенность 5 уровней!
     ├─ РУЧНОЙ ПОЛИВ (40 строк)
     ├─ АВТОМАТИЧЕСКИЙ ПОЛИВ (100 строк)
     ├─ УПРАВЛЕНИЕ ПРИНУДИТЕЛЬНЫМ ПОЛИВОМ (60 строк)
     ├─ УПРАВЛЕНИЕ ОКНАМИ (50 строк)
     ├─ ОТОПЛЕНИЕ (30 строк)
     ├─ НАГРЕВ РАСТВОРА (30 строк)
     ├─ ВЕНТИЛЯЦИЯ (30 строк)
     ├─ ТУМАНООБРАЗОВАНИЕ (120 строк!) - самая сложная часть
     ├─ СОХРАНЕНИЕ ИСТОРИИ (10 строк)
     ├─ MQTT ПУБЛИКАЦИЯ (30 строк)
     └─ vTaskDelay(500) (1 строка)
  }
}
```

**Вложенность logicTask:**
```
for (;;) {
  if (sett.wateringMode == 0) {
    if (currentMin >= startMin && currentMin < endMin) {
      if (sett.pumpState && elapsedWater < durationSec) {
        if (millis() - lastAutoWateringCheck >= sett.radCheckInterval) {
          // 5 уровней вложенности!
        }
      }
    }
  }
}
```

**Вывод**: logicTask нуждается в рефакторинге на подфункции!

---

## 7. ИСПОЛЬЗОВАНИЕ ПАМЯТИ

### 7.1 Статическое использование

**Структуры в памяти:**
```
Settings sett              ≈ 500-600 bytes   (199 полей)
HistoryEntry[20]          ≈ 3.2 KB          (20 записей × ~160 bytes)
Глобальные переменные     ≈ 100 bytes
QueueHandles              ≈ 50 bytes
```

**Итого статическая память: ≈ 4 KB**

### 7.2 Динамическое использование

**DynamicJsonDocument используется 17 раз:**

| Размер | Количество | Описание |
|--------|-----------|---------|
| 5120 bytes | 2 | loadSettings(), POST /settings |
| 4096 bytes | 6 | publishAllSensors(), publishSettings(), saveHistory() |
| 2048 bytes | 0 | не используется |
| 1024 bytes | 1 | /data endpoint |
| 512 bytes | 3 | /cmd/equipment, /cmd/hydro |
| 256 bytes | 5 | разные обработчики |

**Пики памяти:**
- loadSettings: 5120 + входной JSON ≈ 10+ KB
- callback: 4096 ≈ 4KB
- publishSettings: 5120 ≈ 5KB

**Потенциальная проблема**: При одновременном использовании нескольких JSON документов возможен OutOfMemory

### 7.3 Буферы в памяти

```cpp
// Строка 3225 - буфер для POST /settings
static uint8_t jsonBuffer[4096];        // 4 KB в памяти программы

// Строка 4057 - буфер для POST /cmd/window
static String bodyBuffer;               // Динамический, может расти

// Строка 1993 - Modbus буферы
mb.getResponseBuffer();                 // внутри ModbusMaster
```

### 7.4 Строки (String) в памяти

**Проблемные строки:**
- Строка 973: `String msg = String(msg_buffer);` - конвертация буфера в String
- Строка 1667: `String eq = doc["equipment"].as<String>();` - много таких
- HTTP ответы часто создают временные String объекты

**Риск**: Фрагментация heap из-за частого создания/удаления String

---

## 8. ПОТЕНЦИАЛЬНЫЕ ПРОБЛЕМЫ С МНОГОПОТОЧНОСТЬЮ

### 8.1 FreeRTOS задачи

**Текущие задачи:**
```
1. logicTask       (Core 0, пинненная, 12288 bytes stack)
2. modbusTask      (Core 1?, 4096 bytes stack)
3. networkTimeTask (Core 1?, 4096 bytes stack)
4. HTTP обработчики (основной поток)
```

### 8.2 Разделяемые данные (Race Conditions)

**ПРОБЛЕМА 1: Структура sett без полной защиты**

```cpp
// Семафор создан
SemaphoreHandle_t mtx;
mtx = xSemaphoreCreateMutex();          // Строка 3116

// НО: Использование!!!
// Нет ни одного xSemaphoreTake в коде!
// Структура sett читается и пишется без защиты
```

**Race Condition 1**: logicTask пишет в sett, а callback одновременно читает

```cpp
// logicTask() строка 2170
sett.pumpState = true;  // Запись без lock

// callback() строка 989
if (sett.minTemp != val) {  // Чтение без lock - может быть несогласованное состояние!
  sett.minTemp = val;  // Запись без lock
}
```

**Последствия**:
- Потеря обновлений
- Повреждение данных
- Непредсказуемое поведение

**ПРОБЛЕМА 2: Очереди vs структура**

```cpp
// Датчики отправляют в qAvg
xQueueSend(qAvg, &data, 0);

// logicTask читает
xQueuePeek(qAvg, &avg, 0);  // Peek = без удаления

// Но sett обновляется напрямую, без очереди!
sett.pumpState = true;
```

**ПРОБЛЕМА 3: GPIO операции не синхронизированы**

```cpp
// logicTask() строка 2616
if (digitalRead(REL_PUMP) != targetPumpState) {
  digitalWrite(REL_PUMP, targetPumpState ? HIGH : LOW);
}

// HTTP обработчик может одновременно вызвать
digitalWrite(REL_PUMP, HIGH);  // Конфликт!
```

### 8.3 Недостаточная синхронизация

```cpp
volatile bool forceWifiReconnect = false;      // Есть volatile
volatile bool wifiConnectedOnce = false;       // Есть volatile
unsigned long lastNetworkCheckTime = 0;        // НЕ volatile!
unsigned long lastTimeUpdateTime = 0;          // НЕ volatile!

// Задача читает/пишет эти переменные без lock
lastNetworkCheckTime = millis();  // Строка 1862
```

### 8.4 Проблемы с vTaskDelay

```cpp
// modbusTask() - много vTaskDelay(100/portTICK_PERIOD_MS)
for (;;) {
  vTaskDelay(100 / portTICK_PERIOD_MS);
  // Читает/пишет sett без lock
}

// logicTask() - vTaskDelay(500)
for (;;) {
  vTaskDelay(500 / portTICK_PERIOD_MS);
  // Тоже без lock
}
```

---

## 9. СПЕЦИФИЧЕСКИЕ РИСКИ И ПРОБЛЕМЫ

### 9.1 Логика полива (сложная и хрупкая)

**Проблемы:**
1. **Состояние после перезагрузки** (Строки 2164-2249):
   - Попытка восстановить полив если он был активен
   - Много условных проверок
   - Легко нарушить целостность состояния

2. **Утренний полив** (Строки 2250-2360):
   - Циклический полив (до 4 раз)
   - Сложная логика переходов
   - Взаимодействие с forced режимом

3. **Принудительный полив** (Строки 2361-2470):
   - Флаг `forcedWateringPerformed` для отслеживания реального полива
   - Сброс таймеров только если был полив
   - Легко потерять синхронизацию

### 9.2 Логика туманообразования (самая сложная)

**Проблемы:**
1. **3 режима** (Auto, Manual, Forced):
   - Auto (строки 2820-2880): зависит от времени дня
   - Manual (строки 2880-2930): зависит от минут
   - Forced (строки 2810-2820): просто включено/выключено

2. **Задержка включения** (fogDelay):
   - Сначала открывается клапан (fogValveOn)
   - Потом через fogDelay запускается насос
   - Строки 2824-2841 - сложная логика таймеров

3. **Состояние после перезагрузки**:
   - Нет восстановления статуса туманирования
   - Может быть несогласованность

### 9.3 Управление окнами

**Проблемы:**
1. **Потеря позиции** (Строки 1808-1824):
   ```cpp
   // Позиция вычисляется заново каждый раз
   percentage = (float)elapsed / fullTime * 100.0;
   windowPos = constrain(windowPos + percentage, 0, 100);
   ```
   - Это неправильно! Добавляет процент каждый раз

2. **Ручное управление** (sett.manualWindow):
   - Если ручное = true, логика не контролирует окно
   - Но никто не сбрасывает manualWindow = false
   - Окно может остаться в ручном режиме навсегда

### 9.4 Контроль времени

**Проблемы:**
1. **RTC синхронизация** (Строки 1825-1850):
   - NTP обновление каждые 30 минут
   - Если WiFi недоступен, RTC может рассинхронизироваться

2. **Переход на летнее время**:
   - Нет обработки перехода на летнее/зимнее время
   - Все расписания будут смещены на час!

### 9.5 Сохранение настроек

**Проблемы:**
1. **Cooldown 5000мс** (Строка 217):
   - Если много изменений, сохранение может отставать
   - При сбое питания потеря до 5 секунд данных

2. **Проверка "зерое" состояния** (Строка 421):
   ```cpp
   if (sett.minTemp > 0 || sett.maxTemp > 0 || ...)
   ```
   - Очень длинное условие (200+ символов)
   - Не охватывает все поля (например, fogMode может быть 0)

---

## 10. ПОТЕНЦИАЛЬНЫЕ УГРОЗЫ БЕЗОПАСНОСТИ

### 10.1 WiFi и MQTT

```cpp
// Строки 51-55: Твёрдокодированные учётные данные!
const char *WIFI_SSID = "2G_WIFI_05E0";
const char *WIFI_PASS = "12345678";      // ⚠️ ПАРОЛЬ В ИСХОДНИКЕ!
const char *MQTT_BROKER = "broker.emqx.io";
```

**Риск**: Пароль видим в исходнике, может быть скомпилирован в бинарник

### 10.2 Буфер переполнения

```cpp
// Строка 3225: Фиксированный буфер 4096
static uint8_t jsonBuffer[4096];
if (index + len >= sizeof(jsonBuffer)) {  // Проверка есть ✓
  request->send(413);  // Отклоняет, но...
}
```

Проверка есть, но это потенциальный вектор DDoS

### 10.3 JSON парсинг

```cpp
// Нет валидации значений перед использованием
float val = doc["minTemp"];
sett.minTemp = val;  // val может быть NaN или бесконечность!
```

---

## 11. РЕКОМЕНДАЦИИ ПО УЛУЧШЕНИЮ

### Приоритет 1: КРИТИЧЕСКИХ (необходимо исправить сейчас)

1. **Добавить защиту от race conditions**:
   ```cpp
   // Перед доступом к sett
   xSemaphoreTake(mtx, portMAX_DELAY);
   sett.field = value;
   xSemaphoreGive(mtx);
   ```

2. **Рефакторинг logicTask()**:
   - Разделить на подфункции
   - Переместить static переменные в структуру
   - Уменьшить вложенность

3. **Удалить дублирование кода**:
   - Создать `updateSettingFromJson()` функцию
   - Использовать для callback() и POST /settings

### Приоритет 2: ВАЖНЫЕ

4. **Добавить валидацию параметров**:
   ```cpp
   if (isnan(val) || isinf(val)) return false;
   if (val < minValue || val > maxValue) return false;
   ```

5. **Переместить пароли в EEPROM/NVS**:
   - Не хранить в исходнике
   - Загружать при инициализации

6. **Добавить более хорошую обработку ошибок**:
   - Проверять возвращаемые значения функций
   - Логировать ошибки модема

### Приоритет 3: ОПТИМИЗАЦИЯ

7. **Использовать StaticJsonDocument для малых JSON**:
   - Вместо DynamicJsonDocument(256) → StaticJsonDocument<256>

8. **Кэширование часто используемых значений**:
   - `bool isNight()` вызывается много раз в цикле
   - Вычислить один раз за цикл

9. **Оптимизировать использование памяти**:
   - Объединить похожие поля Settings
   - Использовать битовые флаги для boolean

---

## ИТОГОВАЯ СТАТИСТИКА

| Метрика | Значение |
|---------|----------|
| Всего строк кода | 4164 |
| Функции | 27 |
| Среднее строк/функция | 154 |
| Самая большая функция | logicTask (919 строк) |
| Структуры данных | 4 основные |
| Полей в Settings | 199 |
| Глобальных переменных | ~30 |
| Очередей FreeRTOS | 7 |
| Задач FreeRTOS | 3 |
| Web endpoints | 6 |
| MQTT topics | 5 pub + 2 sub |
| Modbus slaves | 7 |
| GPIO реле | 10 |
| DynamicJsonDocument | 17 x разные размеры |
| Потенциальных race conditions | 5+ |
| Дублированного кода | ~600 строк |
| Критических проблем | 3-5 |
