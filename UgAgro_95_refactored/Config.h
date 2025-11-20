// ===========================
// КОНФИГУРАЦИЯ СИСТЕМЫ УМНОЙ ТЕПЛИЦЫ
// ===========================

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===========================
// ОТЛАДКА
// ===========================
#define DEBUG 0
#if DEBUG
  #define LOG(x...) Serial.printf(x)
#else
  #define LOG(...)
#endif

// ===========================
// MODBUS НАСТРОЙКИ
// ===========================
#define MODBUS_RX 16
#define MODBUS_TX 17
#define MODBUS_BAUD 19200

// Адреса Modbus устройств
#define SLAVE_TEMP1 1       // Датчик температуры #1
#define SLAVE_TEMP2 2       // Датчик температуры #2
#define SLAVE_WIND 3        // Датчик ветра
#define SLAVE_PYRANO 5      // Пиранометр (солнечная радиация)
#define SLAVE_MAT_SENSOR 7  // Датчик субстрата (температура, влажность, EC, pH)
#define SLAVE_SOL 12        // Датчик раствора
#define SLAVE_LEVEL 14      // Датчик уровня
#define SLAVE_OUTDOOR 15    // Наружный датчик

// ===========================
// ПИНЫ РЕЛЕ
// ===========================
#define REL_WINDOW_UP 33    // Реле открытия окна
#define REL_WINDOW_DN 32    // Реле закрытия окна
#define REL_HEAT 25         // Реле отопления
#define REL_FAN 26          // Реле вентилятора
#define REL_FOG_VALVE 27    // Реле клапана туманообразования
#define REL_FOG_PUMP 13     // Реле насоса туманообразования
#define REL_PUMP 14         // Реле основного насоса полива
#define REL_FILL 18         // Реле наполнения
#define REL_3WAY 12         // Реле 3-ходового клапана
#define REL_SOL 15          // Реле нагрева раствора

// ===========================
// MQTT НАСТРОЙКИ
// ===========================
#define MQTT_CONNECT_TIMEOUT 3000
#define MQTT_RETRY_INTERVAL_DISCONNECTED 10000
#define MQTT_RETRY_INTERVAL_CONNECTED 60000

// ===========================
// СЕТЕВЫЕ НАСТРОЙКИ
// ===========================
const char* const WIFI_SSID = "2G_WIFI_05E0";
const char* const WIFI_PASS = "12345678";
const char* const MQTT_BROKER = "broker.emqx.io";
const char* const NTP_SERVER = "pool.ntp.org";
const int NTP_TIMEZONE_OFFSET = 10800; // UTC+3 (в секундах)

// ===========================
// ИНТЕРВАЛЫ И ТАЙМАУТЫ
// ===========================
const unsigned long NETWORK_CHECK_INTERVAL = 30000;        // 30 секунд
const unsigned long TIME_UPDATE_INTERVAL = 30 * 60 * 1000; // 30 минут
const unsigned long SAVE_COOLDOWN = 5000;                  // 5 секунд между сохранениями

// ===========================
// РАЗМЕР ИСТОРИИ
// ===========================
#define HISTORY_SIZE 20

// ===========================
// СТРУКТУРЫ ДАННЫХ
// ===========================

// Структура для хранения данных датчиков температуры и влажности
struct SensorData {
  float temperature;
  float humidity;
};

// Структура для датчика субстрата (мат-сенсор)
struct MatSensorData {
  float temperature;
  float humidity;
  float ec;  // Электропроводность
  float ph;  // Кислотность
};

// Структура режима полива
struct WateringMode {
  bool enabled;
  uint8_t startHour;
  uint8_t endHour;
  uint32_t duration;
};

// Запись истории
struct HistoryEntry {
  uint32_t timestamp;
  float avgTemp;
  float avgHum;
  float level;
  float windSpeed;
  float outdoorTemp;
  float outdoorHum;
  float windowPosition;
  bool fanState;
  bool heatingState;
  bool wateringState;
  bool solutionHeatingState;
  bool hydroState;
  bool fillState;
  bool fogState;
};

// Основная структура настроек
struct Settings {
  // Температура и влажность
  float minTemp;
  float maxTemp;
  float minHum;
  float maxHum;

  // Окна
  uint32_t openTime;
  uint32_t closeTime;
  bool manualWindow;
  String windowCommand;
  uint32_t windowDuration;

  // Отопление
  float heatOn;
  float heatOff;
  bool manualHeat;
  bool heatState;

  // Вентилятор
  uint32_t fanInterval;
  uint32_t fanDuration;
  bool manualFan;
  bool fanState;

  // Туманообразование
  uint8_t fogMorningStart;
  uint8_t fogMorningEnd;
  uint8_t fogDayStart;
  uint8_t fogDayEnd;
  uint32_t fogMorningDuration;
  uint32_t fogMorningInterval;
  uint32_t fogDayDuration;
  uint32_t fogDayInterval;
  uint32_t fogDelay;
  float fogMinHum;
  float fogMaxHum;
  float fogMinTemp;
  float fogMaxTemp;
  uint32_t fogMinDuration;
  uint32_t fogMaxDuration;
  uint32_t fogMinInterval;
  uint32_t fogMaxInterval;
  int fogMode;
  bool forceFogOn;
  uint32_t autoFogDayStart;
  uint32_t autoFogDayEnd;
  bool fogState;

  // Раствор и нагрев
  float solOn;
  float solOff;
  bool manualSol;
  bool solHeatState;

  // Ветер
  float wind1;
  float wind2;
  uint32_t windLockMinutes;

  // День/ночь
  uint8_t nightStart;
  uint8_t dayStart;
  float nightOffset;

  // Уровень
  float levelMin;
  float levelMax;
  bool fillState;

  // Гидропоника
  uint32_t hydroMixDuration;
  bool hydroMix;
  uint32_t hydroStart;
  uint32_t hydroStartUnix;

  // Полив
  bool manualPump;
  bool previousManualPump;
  bool manualPumpOverride;
  bool pumpState;
  WateringMode wateringModes[4];
  bool forceWateringActive;
  bool forceWateringOverride;
  uint32_t forceWateringEndTime;
  uint32_t forceWateringEndTimeUnix;
  int wateringMode;
  uint32_t manualWateringInterval;
  uint32_t manualWateringDuration;
  float matMinHumidity;
  float matMaxEC;
  float radThreshold;
  uint32_t pumpTime;
  uint8_t wateringStartHour;
  uint8_t wateringStartMinute;
  uint8_t wateringEndHour;
  uint8_t wateringEndMinute;
  uint8_t maxWateringCycles;
  uint32_t radSumResetInterval;
  uint32_t minWateringInterval;
  uint32_t maxWateringInterval;
  uint32_t forcedWateringDuration;
  int prevWateringMode;
  uint8_t morningWateringCount;
  uint32_t morningWateringInterval;
  uint32_t morningWateringDuration;
  uint32_t radCheckInterval;
  float radSum;
  uint8_t cycleCount;
  uint32_t lastWateringStartUnix;
  uint32_t lastSunTimeUnix;
  uint32_t lastManualWateringTimeUnix;
  bool morningStartedToday;
  bool morningWateringActive;
  uint8_t currentMorningWatering;
  uint32_t lastMorningWateringStartUnix;
  uint32_t lastMorningWateringEndUnix;
  bool pendingMorningComplete;
  bool forcedWateringPerformed; // Флаг реального полива в forced режиме
};

#endif // CONFIG_H
