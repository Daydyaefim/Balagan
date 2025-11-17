#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

/**
 * @brief Режим полива по времени
 */
struct WateringMode {
  bool enabled;
  uint8_t startHour;
  uint8_t endHour;
  uint32_t duration;
};

/**
 * @brief Настройки температуры и влажности
 */
struct ClimateSettings {
  float minTemp;        // Минимальная температура (°C)
  float maxTemp;        // Максимальная температура (°C)
  float minHum;         // Минимальная влажность (%)
  float maxHum;         // Максимальная влажность (%)
};

/**
 * @brief Настройки управления окнами
 */
struct WindowSettings {
  uint32_t openTime;    // Время полного открытия (мс)
  uint32_t closeTime;   // Время полного закрытия (мс)
  bool manualWindow;    // Ручной режим
  String windowCommand; // Команда окна ("open", "close", "stop")
  uint32_t windowDuration; // Длительность команды (мс)
};

/**
 * @brief Настройки отопления
 */
struct HeatingSettings {
  float heatOn;         // Температура включения (°C)
  float heatOff;        // Температура выключения (°C)
  bool manualHeat;      // Ручной режим
  bool heatState;       // Состояние отопления
};

/**
 * @brief Настройки вентилятора
 */
struct FanSettings {
  uint32_t fanInterval; // Интервал работы (мс)
  uint32_t fanDuration; // Длительность работы (мс)
  bool manualFan;       // Ручной режим
  bool fanState;        // Состояние вентилятора
};

/**
 * @brief Настройки системы тумана
 */
struct FogSettings {
  int fogMode;          // Режим: 0=auto, 1=manual, 2=forced

  // Временные интервалы
  uint8_t fogMorningStart;   // Начало утреннего режима (час)
  uint8_t fogMorningEnd;     // Конец утреннего режима (час)
  uint8_t fogDayStart;       // Начало дневного режима (час)
  uint8_t fogDayEnd;         // Конец дневного режима (час)
  uint32_t autoFogDayStart;  // Автоматический старт дневного тумана
  uint32_t autoFogDayEnd;    // Автоматический конец дневного тумана

  // Длительности и интервалы
  uint32_t fogMorningDuration;  // Длительность утром (мс)
  uint32_t fogMorningInterval;  // Интервал утром (мс)
  uint32_t fogDayDuration;      // Длительность днем (мс)
  uint32_t fogDayInterval;      // Интервал днем (мс)
  uint32_t fogDelay;            // Задержка клапана (мс)

  // Параметры адаптивного режима (зависимость от T/H)
  float fogMinHum;          // Минимальная влажность для расчета (%)
  float fogMaxHum;          // Максимальная влажность для расчета (%)
  float fogMinTemp;         // Минимальная температура для расчета (°C)
  float fogMaxTemp;         // Максимальная температура для расчета (°C)
  uint32_t fogMinDuration;  // Минимальная длительность (мс)
  uint32_t fogMaxDuration;  // Максимальная длительность (мс)
  uint32_t fogMinInterval;  // Минимальный интервал (мс)
  uint32_t fogMaxInterval;  // Максимальный интервал (мс)

  // Управление
  bool forceFogOn;       // Принудительное включение
  bool fogState;         // Текущее состояние
};

/**
 * @brief Настройки подогрева раствора
 */
struct SolutionSettings {
  float solOn;          // Температура включения (°C)
  float solOff;         // Температура выключения (°C)
  bool manualSol;       // Ручной режим
  bool solHeatState;    // Состояние подогрева
};

/**
 * @brief Настройки уровня воды
 */
struct LevelSettings {
  float levelMin;       // Минимальный уровень (%)
  float levelMax;       // Максимальный уровень (%)
  bool fillState;       // Состояние заполнения
};

/**
 * @brief Настройки системы полива
 */
struct WateringSettings {
  // Основные режимы
  int wateringMode;     // Режим: 0=auto, 1=manual, 2=timed modes
  WateringMode wateringModes[4]; // 4 режима по времени

  // Базовые параметры
  uint32_t pumpTime;    // Длительность полива (мс)
  bool manualPump;      // Ручной режим
  bool pumpState;       // Состояние насоса
  bool previousManualPump;    // Предыдущее состояние ручного режима
  bool manualPumpOverride;    // Переопределение ручного режима

  // AUTO режим (радиация)
  float radThreshold;         // Порог радиации (W/m²)
  float radSum;               // Накопленная радиация
  uint32_t radCheckInterval;  // Интервал проверки радиации (мс)
  uint32_t radSumResetInterval; // Интервал сброса radSum (мс)
  uint32_t lastSunTimeUnix;   // Время последней проверки радиации

  // Временные окна полива
  uint8_t wateringStartHour;    // Начало окна полива (час)
  uint8_t wateringStartMinute;  // Начало окна полива (минута)
  uint8_t wateringEndHour;      // Конец окна полива (час)
  uint8_t wateringEndMinute;    // Конец окна полива (минута)

  // Ограничения
  uint8_t maxWateringCycles;    // Максимум циклов в день
  uint8_t cycleCount;           // Текущий счетчик циклов
  uint32_t minWateringInterval; // Минимальный интервал между поливами (мс)
  uint32_t maxWateringInterval; // Максимальный интервал между поливами (мс)
  uint32_t lastWateringStartUnix; // Время последнего полива

  // MANUAL режим
  uint32_t manualWateringInterval;  // Интервал ручного полива (мс)
  uint32_t manualWateringDuration;  // Длительность ручного полива (мс)
  uint32_t lastManualWateringTimeUnix; // Время последнего ручного полива

  // MORNING режим (утренняя последовательность)
  uint8_t morningWateringCount;     // Количество утренних поливов
  uint32_t morningWateringInterval; // Интервал между утренними поливами (мс)
  uint32_t morningWateringDuration; // Длительность утреннего полива (мс)
  bool morningStartedToday;         // Флаг: утренний полив начался сегодня
  bool morningWateringActive;       // Флаг: утренний полив активен
  uint8_t currentMorningWatering;   // Текущий номер утреннего полива
  uint32_t lastMorningWateringStartUnix; // Время начала последнего утреннего полива
  uint32_t lastMorningWateringEndUnix;   // Время окончания последнего утреннего полива
  bool pendingMorningComplete;      // Флаг: ожидание завершения утреннего полива

  // FORCED режим (принудительный полив/заполнение)
  bool forceWateringActive;         // Флаг: принудительный полив активен
  bool forceWateringOverride;       // Флаг: переопределение принудительного полива
  uint32_t forceWateringEndTime;    // Время окончания принудительного полива (deprecated)
  uint32_t forceWateringEndTimeUnix; // Время окончания принудительного полива (unix)
  uint32_t forcedWateringDuration;  // Длительность принудительного полива (мс)
  bool forcedWateringPerformed;     // Флаг: выполнен реальный принудительный полив
  int prevWateringMode;             // Предыдущий режим полива (для возврата)

  // Hydro Mix (смешивание раствора)
  bool hydroMix;                // Флаг: режим смешивания
  uint32_t hydroMixDuration;    // Длительность смешивания (мс)
  uint32_t hydroStart;          // Время начала смешивания (deprecated)
  uint32_t hydroStartUnix;      // Время начала смешивания (unix)

  // Датчик мата (условия полива)
  float matMinHumidity;     // Минимальная влажность мата (%)
  float matMaxEC;           // Максимальная EC мата
};

/**
 * @brief Общие настройки системы
 */
struct GeneralSettings {
  // Ветер
  float wind1;              // Порог ветра 1 (м/с)
  float wind2;              // Порог ветра 2 (м/с)
  uint32_t windLockMinutes; // Минуты блокировки при ветре

  // День/ночь
  uint8_t nightStart;   // Начало ночи (час)
  uint8_t dayStart;     // Начало дня (час)
  float nightOffset;    // Смещение температуры ночью (°C)
};

/**
 * @brief Объединенная структура всех настроек
 */
struct Settings {
  ClimateSettings climate;
  WindowSettings window;
  HeatingSettings heating;
  FanSettings fan;
  FogSettings fog;
  SolutionSettings solution;
  LevelSettings level;
  WateringSettings watering;
  GeneralSettings general;

  /**
   * @brief Применить значения по умолчанию
   */
  void setDefaults() {
    // Климат
    climate.minTemp = 18.0;
    climate.maxTemp = 25.0;
    climate.minHum = 50.0;
    climate.maxHum = 70.0;

    // Окна
    window.openTime = 156000;
    window.closeTime = 156000;
    window.manualWindow = false;
    window.windowCommand = "";
    window.windowDuration = 0;

    // Отопление
    heating.heatOn = 18.0;
    heating.heatOff = 20.0;
    heating.manualHeat = false;
    heating.heatState = false;

    // Вентилятор
    fan.fanInterval = 3600000; // 1 час
    fan.fanDuration = 300000;  // 5 минут
    fan.manualFan = false;
    fan.fanState = false;

    // Туман
    fog.fogMode = 0;
    fog.fogMorningStart = 6;
    fog.fogMorningEnd = 12;
    fog.fogDayStart = 12;
    fog.fogDayEnd = 18;
    fog.fogMorningDuration = 5000;
    fog.fogMorningInterval = 300000;
    fog.fogDayDuration = 3000;
    fog.fogDayInterval = 180000;
    fog.fogDelay = 2000;
    fog.fogMinHum = 50.0;
    fog.fogMaxHum = 80.0;
    fog.fogMinTemp = 15.0;
    fog.fogMaxTemp = 30.0;
    fog.fogMinDuration = 2000;
    fog.fogMaxDuration = 10000;
    fog.fogMinInterval = 60000;
    fog.fogMaxInterval = 600000;
    fog.forceFogOn = false;
    fog.fogState = false;
    fog.autoFogDayStart = 0;
    fog.autoFogDayEnd = 0;

    // Раствор
    solution.solOn = 18.0;
    solution.solOff = 20.0;
    solution.manualSol = false;
    solution.solHeatState = false;

    // Уровень
    level.levelMin = 20.0;
    level.levelMax = 80.0;
    level.fillState = false;

    // Полив
    watering.wateringMode = 0;
    for (int i = 0; i < 4; i++) {
      watering.wateringModes[i].enabled = false;
      watering.wateringModes[i].startHour = 0;
      watering.wateringModes[i].endHour = 0;
      watering.wateringModes[i].duration = 0;
    }
    watering.pumpTime = 30000;  // 30 секунд
    watering.manualPump = false;
    watering.pumpState = false;
    watering.previousManualPump = false;
    watering.manualPumpOverride = false;
    watering.radThreshold = 100.0;
    watering.radSum = 0.0;
    watering.radCheckInterval = 60000;  // 1 минута
    watering.radSumResetInterval = 3600000;  // 1 час
    watering.lastSunTimeUnix = 0;
    watering.wateringStartHour = 6;
    watering.wateringStartMinute = 0;
    watering.wateringEndHour = 18;
    watering.wateringEndMinute = 0;
    watering.maxWateringCycles = 10;
    watering.cycleCount = 0;
    watering.minWateringInterval = 1800000;  // 30 минут
    watering.maxWateringInterval = 7200000;  // 2 часа
    watering.lastWateringStartUnix = 0;
    watering.manualWateringInterval = 3600000;  // 1 час
    watering.manualWateringDuration = 30000;    // 30 секунд
    watering.lastManualWateringTimeUnix = 0;
    watering.morningWateringCount = 3;
    watering.morningWateringInterval = 600000;  // 10 минут
    watering.morningWateringDuration = 30000;   // 30 секунд
    watering.morningStartedToday = false;
    watering.morningWateringActive = false;
    watering.currentMorningWatering = 0;
    watering.lastMorningWateringStartUnix = 0;
    watering.lastMorningWateringEndUnix = 0;
    watering.pendingMorningComplete = false;
    watering.forceWateringActive = false;
    watering.forceWateringOverride = false;
    watering.forceWateringEndTime = 0;
    watering.forceWateringEndTimeUnix = 0;
    watering.forcedWateringDuration = 60000;  // 1 минута
    watering.forcedWateringPerformed = false;
    watering.prevWateringMode = 0;
    watering.hydroMix = false;
    watering.hydroMixDuration = 300000;  // 5 минут
    watering.hydroStart = 0;
    watering.hydroStartUnix = 0;
    watering.matMinHumidity = 50.0;
    watering.matMaxEC = 2.5;

    // Общие
    general.wind1 = 5.0;
    general.wind2 = 10.0;
    general.windLockMinutes = 30;
    general.nightStart = 20;
    general.dayStart = 6;
    general.nightOffset = -2.0;
  }
};

#endif // SETTINGS_H
