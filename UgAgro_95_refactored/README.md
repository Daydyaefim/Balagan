# UgAgro_95 Refactored - Этап 3

## Описание

Это рефакторенная версия прошивки для умной теплицы UgAgro_95. Данная версия демонстрирует реализацию **Этапа 3: Command Pattern для MQTT/Web**.

## Проблема в оригинальном коде

Оригинальный код (`../UgAgro_95/UgAgro_95.ino`) содержал массовое дублирование:

### До рефакторинга:
- **callback() функция** (MQTT обработка): **800+ строк**
- **Web handlers** (HTTP обработка): **дублирование 800+ строк**
- **154+ блоков** идентичного кода типа:
  ```cpp
  if (doc.containsKey("minTemp")) {
    float val = doc["minTemp"];
    if (sett.minTemp != val) {
      sett.minTemp = val;
      needSave = true;
    }
  }
  ```
- **ИТОГО**: ~2500+ строк дублированного кода

## Решение: Command Pattern

Паттерн Command инкапсулирует операции над настройками в отдельные объекты, что позволяет:
- Устранить дублирование кода
- Использовать единую обработку для MQTT и Web
- Автоматически валидировать все входные данные
- Легко добавлять новые команды

### После рефакторинга:
- **callback()**: **~10 строк**
- **web handlers**: **~10 строк**
- **Сокращение кода**: **~2000+ строк**

## Архитектура

```
UgAgro_95_refactored/
├── UgAgro_95_refactored.ino    # Главный файл с примерами
├── Settings.h                   # Структуры настроек (разделены на группы)
├── commands/
│   ├── ICommand.h              # Базовый интерфейс команды
│   ├── CommandDispatcher.h     # Диспетчер команд
│   ├── SettingCommands.h       # Шаблонные классы команд
│   └── AllCommands.h           # Регистрация всех команд
└── data/
    └── index.html              # Веб-интерфейс (будет скопирован позже)
```

## Структура Settings

Settings разделена на **9 логических групп**:

1. **ClimateSettings** - температура и влажность (4 поля)
2. **WindowSettings** - управление окнами (5 полей)
3. **HeatingSettings** - отопление (4 поля)
4. **FanSettings** - вентилятор (4 поля)
5. **FogSettings** - система тумана (22 поля)
6. **SolutionSettings** - подогрев раствора (4 поля)
7. **LevelSettings** - уровень воды (3 поля)
8. **WateringSettings** - система полива (40+ полей)
9. **GeneralSettings** - общие параметры (6 полей)

## Использование

### 1. Инициализация

```cpp
#include "Settings.h"
#include "commands/CommandDispatcher.h"
#include "commands/AllCommands.h"

Settings sett;
CommandDispatcher commandDispatcher;
AllCommands* allCommands = nullptr;

void setup() {
  sett.setDefaults();
  allCommands = new AllCommands(sett, commandDispatcher);
  Serial.printf("Зарегистрировано команд: %d\n", allCommands->getCommandCount());
}
```

### 2. MQTT Callback (упрощенная версия)

```cpp
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<2048> doc;
  deserializeJson(doc, payload, length);

  bool needSave = false;
  int executed = commandDispatcher.processDocument(doc, needSave);

  if (needSave) {
    saveSettings();
  }
  publishSettings();
}
```

**Было**: 800+ строк с дублированием
**Стало**: 10 строк

### 3. Web Handler (упрощенная версия)

```cpp
void webHandler(AsyncWebServerRequest *request, JsonVariant &json) {
  JsonDocument& doc = json.as<JsonDocument>();

  bool needSave = false;
  commandDispatcher.processDocument(doc, needSave);

  if (needSave) {
    saveSettings();
  }

  request->send(200, "application/json", "{\"success\":true}");
}
```

**Было**: Дублирование 800+ строк из MQTT callback
**Стало**: 10 строк (использует тот же диспетчер!)

## Примеры команд

### Установка одного параметра

```cpp
StaticJsonDocument<256> doc;
doc["minTemp"] = 20.5;

bool needSave = false;
if (commandDispatcher.dispatch("minTemp", doc, needSave)) {
  Serial.println("Температура установлена");
}
```

### Установка нескольких параметров

```cpp
StaticJsonDocument<512> doc;
doc["minTemp"] = 20.5;
doc["maxTemp"] = 28.0;
doc["minHum"] = 60.0;
doc["maxHum"] = 80.0;

bool needSave = false;
int executed = commandDispatcher.processDocument(doc, needSave);
Serial.printf("Выполнено команд: %d\n", executed);
```

### Автоматическая валидация

```cpp
StaticJsonDocument<256> doc;
doc["minTemp"] = 150.0; // Неверное значение (> 50°C)

bool needSave = false;
if (!commandDispatcher.dispatch("minTemp", doc, needSave)) {
  Serial.println("Ошибка: значение вне диапазона");
}
```

## Зарегистрированные команды

Всего зарегистрировано **90+ команд**:

### Климат (4)
- `minTemp`, `maxTemp`, `minHum`, `maxHum`

### Окна (3)
- `openTime`, `closeTime`, `manualWindow`

### Отопление (4)
- `heatOn`, `heatOff`, `manualHeat`, `heatState`

### Вентилятор (4)
- `fanInterval`, `fanDuration`, `manualFan`, `fanState`

### Туман (22)
- `fogMode`, `fogMorningStart`, `fogMorningEnd`, `fogDayStart`, `fogDayEnd`
- `fogMorningDuration`, `fogMorningInterval`, `fogDayDuration`, `fogDayInterval`
- `fogDelay`, `fogMinHum`, `fogMaxHum`, `fogMinTemp`, `fogMaxTemp`
- `fogMinDuration`, `fogMaxDuration`, `fogMinInterval`, `fogMaxInterval`
- `autoFogDayStart`, `autoFogDayEnd`, `forceFogOn`, `fogState`

### Раствор (4)
- `solOn`, `solOff`, `manualSol`, `solHeatState`

### Уровень (3)
- `levelMin`, `levelMax`, `fillState`

### Полив (40+)
- Базовые: `wateringMode`, `pumpTime`, `manualPump`, `pumpState`, ...
- AUTO режим: `radThreshold`, `radSum`, `matMinHumidity`, `matMaxEC`, ...
- Временные окна: `wateringStartHour`, `wateringStartMinute`, ...
- MANUAL режим: `manualWateringInterval`, `manualWateringDuration`, ...
- MORNING режим: `morningWateringCount`, `morningWateringInterval`, ...
- FORCED режим: `forceWateringActive`, `forcedWateringDuration`, ...
- Hydro Mix: `hydroMix`, `hydroMixDuration`, ...

### Общие (6)
- `wind1`, `wind2`, `windLockMinutes`
- `nightStart`, `dayStart`, `nightOffset`

## Валидация

Каждая команда автоматически валидирует входные данные:

| Тип команды | Валидация |
|-------------|-----------|
| `FloatRangeCommand` | Проверка диапазона (min/max) |
| `UInt8RangeCommand` | Проверка диапазона (0-255) |
| `UInt32Command` | Проверка диапазона |
| `IntRangeCommand` | Проверка диапазона |
| `BoolCommand` | Проверка типа boolean |

Примеры диапазонов:
- Температура: `-20.0` до `50.0` °C
- Влажность: `0.0` до `100.0` %
- Часы: `0` до `23`
- Минуты: `0` до `59`
- Интервалы: `60000` (1 мин) до `86400000` мс (24 часа)

## Компиляция

### Требования
- Arduino IDE 1.8+ или PlatformIO
- ESP32 Board Support Package
- Библиотеки:
  - ArduinoJson (v6+)
  - AsyncTCP
  - ESPAsyncWebServer
  - PubSubClient
  - ModbusMaster
  - RTClib

### Компиляция в Arduino IDE
1. Откройте `UgAgro_95_refactored.ino`
2. Выберите Board: "ESP32 Dev Module"
3. Нажмите "Verify" или "Upload"

### Запуск демонстрации
После загрузки откройте Serial Monitor (115200 baud) для просмотра демонстрации Command Pattern.

## Преимущества

### ✓ Сокращение кода
- Удалено **~2000+ строк** дублированного кода
- callback() сократился с 800+ до 10 строк
- Web handlers сократились с 800+ до 10 строк

### ✓ Централизация
- Единая обработка MQTT и Web команд
- Единая валидация
- Единая точка изменения логики

### ✓ Расширяемость
Добавление новой команды требует всего 1 строку:
```cpp
addCommand(new FloatRangeCommand("newParam", settings.newParam, 0.0, 100.0), dispatcher);
```

### ✓ Читаемость
- Четкая структура
- Разделение на логические модули
- Самодокументируемый код

### ✓ Тестируемость
- Команды легко тестировать изолированно
- Моки для Settings
- Unit тесты для валидации

### ✓ Безопасность
- Автоматическая валидация всех входных данных
- Защита от выхода за границы диапазонов
- Защита от некорректных типов

## Дальнейшие улучшения

### Этап 4: State Machine для полива
- Упростить сложную логику полива (~500 строк)
- Четкие состояния и переходы
- Устранение флагов

### Этап 5: Unit тесты
- Покрытие тестами > 70%
- Автоматический запуск в CI
- Моки для аппаратных компонентов

## Сравнение с оригиналом

| Метрика | Оригинал | Рефакторинг | Улучшение |
|---------|----------|-------------|-----------|
| Размер callback() | 800+ строк | ~10 строк | -98.75% |
| Размер web handlers | 800+ строк | ~10 строк | -98.75% |
| Дублирование кода | 154+ блоков | 0 блоков | -100% |
| Валидация | Частичная | Полная | +100% |
| Добавление команды | 8+ мест правок | 1 строка | -87.5% |
| Читаемость | Низкая | Высокая | +++ |
| Тестируемость | Нет | Да | +++ |

## Авторы

- Оригинальный код: UgAgro_95 Team
- Рефакторинг (Этап 3): Claude AI Assistant
- Дата: 2025-11-17

## Лицензия

Смотрите оригинальную лицензию проекта UgAgro_95.

---

**Статус**: Этап 3 завершен ✓
**Следующий этап**: State Machine для полива (Этап 4)
