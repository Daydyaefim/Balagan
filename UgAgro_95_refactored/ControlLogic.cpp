// ===========================
// МОДУЛЬ ЛОГИКИ УПРАВЛЕНИЯ - РЕАЛИЗАЦИЯ (БАЗОВАЯ СТРУКТУРА)
// ===========================

#include "ControlLogic.h"

// Константы
const float GreenhouseController::TEMP_HYSTERESIS = 0.2f;
const float GreenhouseController::HUM_HYSTERESIS = 2.0f;

// Конструктор
GreenhouseController::GreenhouseController(SensorManager& sensors, ActuatorManager& actuators,
                                           MqttManager& mqtt, StorageManager& storage,
                                           RTC_DS3231& rtc, SemaphoreHandle_t mutex)
  : _sensors(sensors), _actuators(actuators), _mqtt(mqtt), _storage(storage),
    _rtc(rtc), _mutex(mutex), _historyIndex(0),
    _qAvg(nullptr), _qSol(nullptr), _qLevel(nullptr), _qWind(nullptr),
    _qPyrano(nullptr), _qMat(nullptr), _qOutdoor(nullptr),
    _heat(false), _fan(false), _fog(false), _pump(false), _solHeat(false),
    _windLockStart(0), _windRestricted(false), _prevWindRestricted(false),
    _lastHistory(0), _lastHour(0), _lastWater(0), _taskFogStartTime(0),
    _taskLastFog(0), _fogValveOn(false), _fogValveStartTime(0),
    _lastFanChange(0), _lastHeatChange(0), _lastSolHeatChange(0),
    _lastWindowMoveCommand(0), _sensorsValid(false), _lastManualHeatCheck(0),
    _lastManualFanCheck(0), _lastManualSolCheck(0), _lastManualPumpCheck(0),
    _lastDay(-1), _lastRadCheck(0), _lastAvgTemp(NAN), _lastAvgHum(NAN) {
}

// Инициализация
void GreenhouseController::begin() {
  // Загрузка настроек из хранилища
  if (!_storage.loadSettings(_settings)) {
    LOG("GreenhouseController: Применены настройки по умолчанию\n");
    _storage.applyDefaultSettings(_settings);
    _storage.saveSettings(_settings, "init_default");
  }

  // Загрузка истории
  int historyCount = 0;
  _storage.loadHistory(_history, historyCount, HISTORY_SIZE);
  _historyIndex = historyCount;

  LOG("GreenhouseController: Инициализация завершена\n");
}

// Установка очередей
void GreenhouseController::setQueues(QueueHandle_t qAvg, QueueHandle_t qSol, QueueHandle_t qLevel,
                                      QueueHandle_t qWind, QueueHandle_t qPyrano, QueueHandle_t qMat,
                                      QueueHandle_t qOutdoor) {
  _qAvg = qAvg;
  _qSol = qSol;
  _qLevel = qLevel;
  _qWind = qWind;
  _qPyrano = qPyrano;
  _qMat = qMat;
  _qOutdoor = qOutdoor;
}

// Проверка, ночное ли время
bool GreenhouseController::isNight() const {
  DateTime now = _rtc.now();
  int h = now.hour();
  return h < _settings.dayStart || h >= _settings.nightStart;
}

// Проверка времени в диапазоне
bool GreenhouseController::isTimeInRange(uint8_t startHour, uint8_t endHour) const {
  DateTime now = _rtc.now();
  int h = now.hour();
  return h >= startHour && h < endHour;
}

// Расчет длительности тумана
float GreenhouseController::calcFogDuration(float hum, float temp) const {
  float humDuration = map((int)hum, (int)_settings.fogMinHum, (int)_settings.fogMaxHum,
                          (int)_settings.fogMaxDuration, (int)_settings.fogMinDuration);
  float tempDuration = map((int)temp, (int)_settings.fogMinTemp, (int)_settings.fogMaxTemp,
                           (int)_settings.fogMinDuration, (int)_settings.fogMaxDuration);
  return max(humDuration, tempDuration);
}

// Расчет интервала тумана
float GreenhouseController::calcFogInterval(float hum, float temp) const {
  float humInterval = map((int)hum, (int)_settings.fogMinHum, (int)_settings.fogMaxHum,
                          (int)_settings.fogMinInterval, (int)_settings.fogMaxInterval);
  float tempInterval = map((int)temp, (int)_settings.fogMinTemp, (int)_settings.fogMaxTemp,
                           (int)_settings.fogMaxInterval, (int)_settings.fogMinInterval);
  return min(humInterval, tempInterval);
}

// Добавление записи в историю
void GreenhouseController::addToHistory(float temp, float hum) {
  float level = 0, wind = 0, outdoorTemp = 0, outdoorHum = 0;

  if (_qLevel) xQueuePeek(_qLevel, &level, 0);
  if (_qWind) xQueuePeek(_qWind, &wind, 0);

  SensorData outdoor;
  if (_qOutdoor && xQueuePeek(_qOutdoor, &outdoor, 0) == pdTRUE) {
    outdoorTemp = outdoor.temperature;
    outdoorHum = outdoor.humidity;
  }

  _history[_historyIndex] = {
    millis(),
    temp,
    hum,
    level,
    wind,
    outdoorTemp,
    outdoorHum,
    (float)_actuators.getWindowPosition(),
    _settings.fanState,
    _settings.heatState,
    _settings.pumpState,
    _settings.solHeatState,
    _settings.hydroMix,
    _settings.fillState,
    _settings.fogState
  };

  _historyIndex = (_historyIndex + 1) % HISTORY_SIZE;
}

// Публикация истории при необходимости
void GreenhouseController::publishHistoryIfNeeded(uint32_t now) {
  DateTime rtcNow = _rtc.now();
  uint32_t currentHour = rtcNow.hour();

  if (now - _lastHistory >= 60000 && currentHour != _lastHour) { // Каждый час
    _mqtt.publishHistory(_history, HISTORY_SIZE);
    _storage.saveHistory(_history, HISTORY_SIZE);
    _lastHistory = now;
    _lastHour = currentHour;
  }
}

// Проверка ручных режимов
void GreenhouseController::checkManualModes(uint32_t now) {
  // ПРИМЕЧАНИЕ: Здесь должна быть логика проверки ручных режимов
  // Это упрощенная версия - полная реализация требует переноса всей логики из исходного кода
}

// Управление отоплением
void GreenhouseController::updateHeating(float temp, float sol) {
  // ПРИМЕЧАНИЕ: Упрощенная версия - полная логика из исходного кода очень сложная
  bool newHeatState = false;

  if (!_settings.manualHeat) {
    // Автоматический режим
    if (temp < _settings.heatOn - TEMP_HYSTERESIS) {
      newHeatState = true;
    } else if (temp > _settings.heatOff + TEMP_HYSTERESIS) {
      newHeatState = false;
    } else {
      newHeatState = _heat; // Гистерезис
    }
  } else {
    // Ручной режим
    newHeatState = _settings.heatState;
  }

  if (newHeatState != _heat) {
    _heat = newHeatState;
    _settings.heatState = newHeatState;
    _actuators.setHeating(newHeatState);
  }
}

// Управление вентилятором
void GreenhouseController::updateFan(uint32_t now) {
  // ПРИМЕЧАНИЕ: Упрощенная версия
  if (!_settings.manualFan) {
    // Логика автоматического управления вентилятором
    // Полная реализация требует переноса логики из исходного кода
  } else {
    _actuators.setFan(_settings.fanState);
  }
}

// Управление нагревом раствора
void GreenhouseController::updateSolutionHeating(float sol) {
  // ПРИМЕЧАНИЕ: Упрощенная версия
  bool newSolHeatState = false;

  if (!_settings.manualSol) {
    if (sol < _settings.solOn - TEMP_HYSTERESIS) {
      newSolHeatState = true;
    } else if (sol > _settings.solOff + TEMP_HYSTERESIS) {
      newSolHeatState = false;
    } else {
      newSolHeatState = _solHeat; // Гистерезис
    }
  } else {
    newSolHeatState = _settings.solHeatState;
  }

  if (newSolHeatState != _solHeat) {
    _solHeat = newSolHeatState;
    _settings.solHeatState = newSolHeatState;
    _actuators.setSolutionHeating(newSolHeatState);
  }
}

// Управление туманом
void GreenhouseController::updateFogging(float hum, float temp, uint32_t now) {
  // ПРИМЕЧАНИЕ: Упрощенная версия - полная логика очень сложная
  // Здесь должна быть логика туманообразования из исходного кода
}

// Управление поливом
void GreenhouseController::updateWatering(float pyrano, const MatSensorData& mat, uint32_t now) {
  // ПРИМЕЧАНИЕ: Упрощенная версия - полная логика полива очень сложная
  // Здесь должна быть логика полива из исходного кода
}

// Управление ограничением ветра
void GreenhouseController::updateWindRestriction(float wind, uint32_t now) {
  // ПРИМЕЧАНИЕ: Упрощенная версия
  if (wind > _settings.wind2 && !_windRestricted) {
    _windRestricted = true;
    _windLockStart = now;
    LOG("GreenhouseController: Ветер выше порога wind2, активирована блокировка\n");
  } else if (wind < _settings.wind1 && _windRestricted) {
    if (now - _windLockStart > _settings.windLockMinutes * 60 * 1000UL) {
      _windRestricted = false;
      LOG("GreenhouseController: Ветер ниже порога wind1, блокировка снята\n");
    }
  }
}

// Управление окнами
void GreenhouseController::updateWindowControl(float temp, float hum, uint32_t now) {
  // ПРИМЕЧАНИЕ: Упрощенная версия
  // Полная логика управления окнами должна быть перенесена из исходного кода
}

// Основной цикл обновления
void GreenhouseController::update() {
  uint32_t now = millis();

  // Чтение данных с датчиков
  float avgTemp = 0, avgHum = 0, sol = 0, level = 0, wind = 0, pyrano = 0;
  SensorData avgData;
  MatSensorData matData;

  _sensorsValid = false;

  if (_qAvg && xQueuePeek(_qAvg, &avgData, 0) == pdTRUE) {
    avgTemp = avgData.temperature;
    avgHum = avgData.humidity;
    _sensorsValid = !isnan(avgTemp) && !isnan(avgHum);
  }

  if (_qSol) xQueuePeek(_qSol, &sol, 0);
  if (_qLevel) xQueuePeek(_qLevel, &level, 0);
  if (_qWind) xQueuePeek(_qWind, &wind, 0);
  if (_qPyrano) xQueuePeek(_qPyrano, &pyrano, 0);
  if (_qMat) xQueuePeek(_qMat, &matData, 0);

  // Обновление позиции окна
  _actuators.updateWindowPosition(now);

  // Если датчики валидны, выполняем управление
  if (_sensorsValid) {
    // Проверка ручных режимов
    checkManualModes(now);

    // Управление подсистемами
    updateHeating(avgTemp, sol);
    updateFan(now);
    updateSolutionHeating(sol);
    updateFogging(avgHum, avgTemp, now);
    updateWatering(pyrano, matData, now);
    updateWindRestriction(wind, now);
    updateWindowControl(avgTemp, avgHum, now);

    // Добавление в историю
    if (avgTemp != _lastAvgTemp || avgHum != _lastAvgHum) {
      addToHistory(avgTemp, avgHum);
      _lastAvgTemp = avgTemp;
      _lastAvgHum = avgHum;
    }
  }

  // Публикация истории
  publishHistoryIfNeeded(now);

  // Публикация данных в MQTT
  if (_mqtt.isConnected()) {
    static uint32_t lastPublish = 0;
    if (now - lastPublish > 5000) { // Каждые 5 секунд
      SensorData outdoor;
      if (_qOutdoor) xQueuePeek(_qOutdoor, &outdoor, 0);

      _mqtt.publishAllSensors(avgData, sol, level, wind, outdoor, matData, pyrano,
                               _actuators.getWindowPosition(), _settings);
      lastPublish = now;
    }
  }

  // Задержка
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// Задача FreeRTOS для логики управления
void logicTask(void* parameter) {
  GreenhouseController* controller = (GreenhouseController*)parameter;

  for (;;) {
    controller->update();
  }
}
