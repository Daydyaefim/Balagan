// ===========================
// МОДУЛЬ ЛОГИКИ УПРАВЛЕНИЯ ТЕПЛИЦЕЙ
// ===========================

#ifndef CONTROL_LOGIC_H
#define CONTROL_LOGIC_H

#include <Arduino.h>
#include <RTClib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "Config.h"
#include "Sensors.h"
#include "Actuators.h"
#include "MqttManager.h"
#include "Storage.h"

// Класс для управления логикой теплицы
class GreenhouseController {
public:
  // Конструктор
  GreenhouseController(SensorManager& sensors, ActuatorManager& actuators,
                       MqttManager& mqtt, StorageManager& storage,
                       RTC_DS3231& rtc, SemaphoreHandle_t mutex);

  // Инициализация
  void begin();

  // Основной цикл управления (вызывается из logicTask)
  void update();

  // Получить настройки
  Settings& getSettings() { return _settings; }

  // Получить историю
  HistoryEntry* getHistory() { return _history; }
  int getHistoryIndex() const { return _historyIndex; }

  // Установить очереди датчиков
  void setQueues(QueueHandle_t qAvg, QueueHandle_t qSol, QueueHandle_t qLevel,
                 QueueHandle_t qWind, QueueHandle_t qPyrano, QueueHandle_t qMat,
                 QueueHandle_t qOutdoor);

private:
  // Ссылки на менеджеры
  SensorManager& _sensors;
  ActuatorManager& _actuators;
  MqttManager& _mqtt;
  StorageManager& _storage;
  RTC_DS3231& _rtc;
  SemaphoreHandle_t _mutex;

  // Настройки и история
  Settings _settings;
  HistoryEntry _history[HISTORY_SIZE];
  int _historyIndex;

  // Очереди для данных датчиков
  QueueHandle_t _qAvg, _qSol, _qLevel, _qWind, _qPyrano, _qMat, _qOutdoor;

  // Состояние подсистем
  bool _heat;
  bool _fan;
  bool _fog;
  bool _pump;
  bool _solHeat;

  // Временные переменные
  uint32_t _windLockStart;
  bool _windRestricted;
  bool _prevWindRestricted;
  uint32_t _lastHistory;
  uint32_t _lastHour;
  uint32_t _lastWater;
  uint32_t _taskFogStartTime;
  uint32_t _taskLastFog;
  bool _fogValveOn;
  uint32_t _fogValveStartTime;
  uint32_t _lastFanChange;
  uint32_t _lastHeatChange;
  uint32_t _lastSolHeatChange;
  uint32_t _lastWindowMoveCommand;
  bool _sensorsValid;
  uint32_t _lastManualHeatCheck;
  uint32_t _lastManualFanCheck;
  uint32_t _lastManualSolCheck;
  uint32_t _lastManualPumpCheck;
  int _lastDay;
  uint32_t _lastRadCheck;
  float _lastAvgTemp;
  float _lastAvgHum;
  uint32_t _fanStartTimestamp;
  bool _initialized;
  bool _forcedPumpStarted;
  uint32_t _lastMqttCheck;
  uint32_t _lastFillChange;

  // Константы
  static const uint32_t MIN_STATE_CHANGE_INTERVAL = 5000;
  static const uint32_t MIN_WINDOW_STATE_CHANGE_INTERVAL = 30000;
  static const float TEMP_HYSTERESIS;
  static const float HUM_HYSTERESIS;

  // Методы управления подсистемами
  void updateHeating(float temp, float sol);
  void updateFan(uint32_t now);
  void updateSolutionHeating(float sol);
  void updateFogging(float hum, float temp, uint32_t now);
  void updateWatering(float pyrano, const MatSensorData& mat, uint32_t now);
  void updateWindRestriction(float wind, uint32_t now);
  void updateWindowControl(float temp, float hum, uint32_t now);

  // Вспомогательные методы
  bool isNight() const;
  bool isTimeInRange(uint8_t startHour, uint8_t endHour) const;
  void addToHistory(float temp, float hum);
  void publishHistoryIfNeeded(uint32_t now);
  void checkManualModes(uint32_t now);
  void initializeWateringState();

  // Вспомогательные функции расчета
  float calcFogDuration(float hum, float temp) const;
  float calcFogInterval(float hum, float temp) const;
  float mapFloat(float x, float inMin, float inMax, float outMin, float outMax) const;
};

// Задача FreeRTOS для логики управления
void logicTask(void* parameter);

#endif // CONTROL_LOGIC_H
