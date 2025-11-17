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
    _lastFanChange(0), _fanStartTimestamp(0), _lastHeatChange(0), _lastSolHeatChange(0),
    _lastWindowMoveCommand(0), _sensorsValid(false), _lastManualHeatCheck(0),
    _lastManualFanCheck(0), _lastManualSolCheck(0), _lastManualPumpCheck(0),
    _lastDay(-1), _lastRadCheck(0), _lastAvgTemp(NAN), _lastAvgHum(NAN),
    _lastAutoWateringCheck(0), _lastManualPeriodicCheck(0), _forcedPumpStarted(false) {
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

// Маппинг float значений
float GreenhouseController::mapFloat(float x, float inMin, float inMax, float outMin, float outMax) const {
  if (inMax == inMin) return outMin;
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
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
  uint32_t now = millis();

  // Автоматический режим с учетом времени между изменениями состояния
  if (!_settings.manualHeat && _sensorsValid && (now - _lastHeatChange > MIN_STATE_CHANGE_INTERVAL)) {
    float on = _settings.heatOn;
    float off = _settings.heatOff;

    // Учет ночного смещения температуры
    if (isNight()) {
      on -= _settings.nightOffset;
      off -= _settings.nightOffset;
    }

    bool newHeatState = _heat;

    // Включение отопления при достижении нижнего порога
    if (temp <= on && !_heat) {
      newHeatState = true;
      Serial.println("Heating turned ON");
      _lastHeatChange = now;
    }
    // Выключение отопления при достижении верхнего порога
    else if (temp >= off && _heat) {
      newHeatState = false;
      Serial.println("Heating turned OFF");
      _lastHeatChange = now;
    }

    // Обновление состояния и сохранение настроек
    if (_settings.heatState != newHeatState) {
      _settings.heatState = newHeatState;
      _heat = newHeatState;
      _storage.saveSettings(_settings, "heating");
    }

    _actuators.setHeating(newHeatState);
  }
  // Отключение при недоступности датчиков
  else if (!_sensorsValid && _heat) {
    _actuators.setHeating(false);
    _heat = false;

    if (_settings.heatState != false) {
      _settings.heatState = false;
      _storage.saveSettings(_settings, "heating_no_sensors");
    }
  }
  // Ручной режим
  else if (_settings.manualHeat) {
    _actuators.setHeating(_settings.heatState);
    _heat = _settings.heatState;
  }
}

// Управление вентилятором
void GreenhouseController::updateFan(uint32_t now) {
  bool targetFan = _fan;

  if (_settings.manualFan) {
    // Ручной режим
    targetFan = _settings.fanState;
  } else {
    // Автоматический режим

    // Включение вентилятора при активном тумане
    bool shouldTurnFanOnForFog = false;
    if (!_fan && _fog) {
      shouldTurnFanOnForFog = true;
    }

    // Включение вентилятора по расписанию (интервалу)
    bool shouldTurnFanOnBySchedule = false;
    if (!_fan && !_fog) {
      if (now - _lastFanChange >= _settings.fanInterval) {
        shouldTurnFanOnBySchedule = true;
      }
    }

    // Выключение вентилятора по длительности работы
    bool shouldTurnFanOffByDuration = false;
    if (_fan) {
      if (now - _fanStartTimestamp >= _settings.fanDuration) {
        shouldTurnFanOffByDuration = true;
      }
    }

    // Определение целевого состояния
    if (shouldTurnFanOnForFog) {
      targetFan = true;
    } else if (shouldTurnFanOffByDuration) {
      targetFan = false;
    } else if (shouldTurnFanOnBySchedule) {
      targetFan = true;
    }
  }

  // Применение изменений состояния
  if (targetFan != _fan) {
    _fan = targetFan;
    _actuators.setFan(_fan);

    if (_fan) {
      _fanStartTimestamp = now;
    }
    _lastFanChange = now;

    if (_settings.fanState != _fan) {
      _settings.fanState = _fan;
      _storage.saveSettings(_settings, "fan_state_changed_by_logic");
      LOG("DEBUG: Fan state changed to %s. lastFanChange updated to %lu\n",
          _fan ? "ON" : "OFF", _lastFanChange);
    }
  }
}

// Управление нагревом раствора
void GreenhouseController::updateSolutionHeating(float sol) {
  uint32_t now = millis();
  float level = 0;
  if (_qLevel) xQueuePeek(_qLevel, &level, 0);

  // Автоматический режим с учетом уровня раствора и времени между изменениями
  if (!_settings.manualSol && level > 20 && _sensorsValid &&
      (now - _lastSolHeatChange > MIN_STATE_CHANGE_INTERVAL)) {
    float on = _settings.solOn;
    float off = _settings.solOff;

    // Учет ночного смещения температуры
    if (isNight()) {
      on -= _settings.nightOffset;
      off -= _settings.nightOffset;
    }

    bool newSolHeatState = _solHeat;

    // Включение нагрева при достижении нижнего порога
    if (sol <= on && !_solHeat) {
      newSolHeatState = true;
      Serial.println("Solution heating turned ON");
      _lastSolHeatChange = now;
    }
    // Выключение нагрева при достижении верхнего порога
    else if (sol >= off && _solHeat) {
      newSolHeatState = false;
      Serial.println("Solution heating turned OFF");
      _lastSolHeatChange = now;
    }

    // Обновление состояния и сохранение настроек
    if (_settings.solHeatState != newSolHeatState) {
      _settings.solHeatState = newSolHeatState;
      _solHeat = newSolHeatState;
      _storage.saveSettings(_settings, "solution_heating");
    }

    _actuators.setSolutionHeating(newSolHeatState);
  }
  // Отключение при недоступности датчиков или низком уровне
  else if (!_sensorsValid && _solHeat) {
    _actuators.setSolutionHeating(false);
    _solHeat = false;

    if (_settings.solHeatState != false) {
      _settings.solHeatState = false;
      _storage.saveSettings(_settings, "solution_heating_no_sensors");
    }
  }
  // Ручной режим
  else if (_settings.manualSol) {
    _actuators.setSolutionHeating(_settings.solHeatState);
    _solHeat = _settings.solHeatState;
  }
}

// Управление туманом
void GreenhouseController::updateFogging(float hum, float temp, uint32_t now) {
  DateTime nowTime = _rtc.now();
  int h = nowTime.hour();
  int m = nowTime.minute();

  // Определение временных окон
  bool morning = (h >= _settings.fogMorningStart && h < _settings.fogMorningEnd);
  bool day = (h >= _settings.autoFogDayStart && h < _settings.autoFogDayEnd);

  uint32_t dur = 0;   // Длительность тумана
  uint32_t intv = 0;  // Интервал между циклами

  // ==========================================
  // РЕЖИМ 2: FORCED - Принудительное управление
  // ==========================================
  if (_settings.fogMode == 2) {
    if (_settings.forceFogOn && !_fog) {
      // Запуск цикла туманообразования
      if (!_fogValveOn) {
        // Сначала открываем клапан
        _actuators.setFogValve(true);
        _fogValveOn = true;
        _fogValveStartTime = now;
        Serial.println("Fog valve turned ON in forced mode");
      } else if (now - _fogValveStartTime >= _settings.fogDelay * 1000UL) {
        // Через задержку включаем насос
        _actuators.setFogPump(true);
        _fog = true;
        _fogValveOn = false;

        if (_settings.fogState != true) {
          _settings.fogState = true;
          _storage.saveSettings(_settings, "fog_forced");
        }
      }
    } else if (!_settings.forceFogOn && _fog) {
      // Остановка туманообразования
      _actuators.setFogPump(false);
      vTaskDelay(50 / portTICK_PERIOD_MS);
      _actuators.setFogValve(false);
      _fog = false;
      _fogValveOn = false;

      if (_settings.fogState != false) {
        _settings.fogState = false;
        _storage.saveSettings(_settings, "fog_forced");
      }
    }
  }
  // ==========================================
  // РЕЖИМ 1: MANUAL - Ручной по расписанию
  // ==========================================
  else if (_settings.fogMode == 1) {
    // Определение параметров в зависимости от времени
    if (morning) {
      dur = _settings.fogMorningDuration;
      intv = _settings.fogMorningInterval;
    } else if (day) {
      dur = _settings.fogDayDuration;
      intv = _settings.fogDayInterval;
    }

    // Запуск цикла, если прошел интервал
    if ((morning || day) && (now - _taskLastFog > intv) && !_fog) {
      if (!_fogValveOn) {
        // Сначала открываем клапан
        _actuators.setFogValve(true);
        _fogValveOn = true;
        _fogValveStartTime = now;
        Serial.println("Fog valve turned ON in manual mode");
      } else if (now - _fogValveStartTime >= _settings.fogDelay * 1000UL) {
        // Через задержку включаем насос
        _actuators.setFogPump(true);
        _fog = true;
        _fogValveOn = false;

        if (_settings.fogState != true) {
          _settings.fogState = true;
          _taskFogStartTime = now;
          _storage.saveSettings(_settings, "fog_manual");
        }
      }
    }

    // Остановка по истечении длительности
    if (_fog && (now - _taskFogStartTime > dur)) {
      _actuators.setFogPump(false);
      _actuators.setFogValve(false);
      _fog = false;
      _fogValveOn = false;

      if (_settings.fogState != false) {
        _settings.fogState = false;
        _taskLastFog = now;
        _storage.saveSettings(_settings, "fog_manual");
      }
    }
  }
  // ==========================================
  // РЕЖИМ 0: AUTO - Автоматический с учетом условий
  // ==========================================
  else if (_settings.fogMode == 0) {
    bool shouldRun = false;

    if (morning) {
      // Утренний режим - всегда работает
      dur = _settings.fogMorningDuration;
      intv = _settings.fogMorningInterval;
      shouldRun = true;
    } else if (day) {
      // Дневной режим - учитываем температуру и влажность
      float validHum = hum;
      float validTemp = temp;

      if (isnan(validHum) || isnan(validTemp)) {
        validHum = _settings.fogMinHum + 1;
        validTemp = (_settings.fogMinTemp + _settings.fogMaxTemp) / 2;
        Serial.println("Warning: Invalid avgHum or avgTemp, using defaults");
      }

      dur = calcFogDuration(validHum, validTemp);
      intv = calcFogInterval(validHum, validTemp);

      // Проверка условий запуска
      shouldRun = (validHum < _settings.fogMaxHum &&
                   validTemp >= _settings.fogMinTemp &&
                   validTemp <= _settings.fogMaxTemp);
    }

    // Запуск цикла, если условия выполнены
    if (shouldRun && (now - _taskLastFog > intv) && !_fog) {
      if (!_fogValveOn) {
        // Сначала открываем клапан
        _actuators.setFogValve(true);
        _fogValveOn = true;
        _fogValveStartTime = now;
        Serial.println("Fog valve turned ON in auto mode");
      } else if (now - _fogValveStartTime >= _settings.fogDelay * 1000UL) {
        // Через задержку включаем насос
        _actuators.setFogPump(true);
        _fog = true;
        _fogValveOn = false;

        if (_settings.fogState != true) {
          _settings.fogState = true;
          _taskFogStartTime = now;
          _storage.saveSettings(_settings, "fog_auto");
        }
      }
    }

    // Остановка по истечении длительности
    if (_fog && (now - _taskFogStartTime > dur)) {
      _actuators.setFogPump(false);
      _actuators.setFogValve(false);
      _fog = false;
      _fogValveOn = false;

      if (_settings.fogState != false) {
        _settings.fogState = false;
        _taskLastFog = now;
        _storage.saveSettings(_settings, "fog_auto");
      }
    }
  }

  // ==========================================
  // Остановка тумана вне временных окон (для режимов 0 и 1)
  // ==========================================
  if ((_settings.fogMode == 0 || _settings.fogMode == 1) && !(morning || day) && _fog) {
    _actuators.setFogPump(false);
    _actuators.setFogValve(false);
    _fog = false;
    _fogValveOn = false;

    if (_settings.fogState != false) {
      _settings.fogState = false;
      _storage.saveSettings(_settings, "fog_outside_window");
    }
  }
}

// Управление поливом
void GreenhouseController::updateWatering(float pyrano, const MatSensorData& mat, uint32_t now) {
  /*
   * ПРИМЕЧАНИЕ: Это базовая реализация логики полива.
   * Полная логика из оригинального кода включает:
   * - Инициализацию состояния при перезагрузке (~200 строк)
   * - Morning watering с несколькими циклами (~100 строк)
   * - Auto watering по радиации с множеством проверок (~150 строк)
   * - Manual watering по интервалу (~30 строк)
   * - Forced watering с hydro mix и fill (~150 строк)
   * - Управление реле pump, 3-way valve, fill valve
   *
   * Для полной миграции требуется дополнительная работа.
   */

  DateTime nowTime = _rtc.now();
  uint32_t nowUnix = nowTime.unixtime();
  int h = nowTime.hour();
  int m = nowTime.minute();
  int currentMin = h * 60 + m;
  int startMin = _settings.wateringStartHour * 60 + _settings.wateringStartMinute;
  int endMin = _settings.wateringEndHour * 60 + _settings.wateringEndMinute;
  int currentDay = nowTime.day();

  // Сброс счетчика циклов при смене дня
  if (currentDay != _lastDay) {
    _settings.cycleCount = 0;
    _settings.morningStartedToday = false;
    _settings.morningWateringActive = false;
    _settings.currentMorningWatering = 0;
    _settings.lastMorningWateringEndUnix = 0;
    _lastDay = currentDay;
    LOG("DEBUG AUTO-WATER: New day started. Reset cycleCount=%d\n", _settings.cycleCount);
    _storage.saveSettings(_settings, "new_day_reset");
  }

  float level = 0;
  if (_qLevel) xQueuePeek(_qLevel, &level, 0);

  // ==========================================
  // РЕЖИМ 0: AUTO WATERING (Полив по радиации)
  // ==========================================
  if (_settings.wateringMode == 0 && !_settings.forceWateringActive &&
      !_settings.forceWateringOverride && !_settings.morningWateringActive) {

    // Периодическая проверка накопления радиации
    if (now - _lastAutoWateringCheck >= _settings.radCheckInterval) {
      _lastAutoWateringCheck = now;

      // Накопление радиации
      if (!isnan(pyrano) && pyrano > 0) {
        float deltaRad = pyrano * (_settings.radCheckInterval / 1000.0f);
        _settings.radSum += deltaRad;
        _settings.lastSunTimeUnix = nowUnix;
        LOG("DEBUG AUTO-WATER: Radiation accumulation: pyrano=%.2f W/m2, delta=%.2f J/m2, new radSum=%.2f J/m2\n",
            pyrano, deltaRad, _settings.radSum);
      } else {
        // Сброс radSum при отсутствии солнца
        uint32_t resetSec = _settings.radSumResetInterval / 1000UL;
        if ((nowUnix - _settings.lastSunTimeUnix) >= resetSec) {
          _settings.radSum = 0.0f;
          _settings.lastSunTimeUnix = nowUnix;
          LOG("DEBUG AUTO-WATER: radSum reset to 0 due to no sun for %u sec\n", resetSec);
        }
      }

      // Проверка условий для запуска полива
      if (currentMin >= startMin && currentMin < endMin) {
        if (level >= 2 && _settings.cycleCount < _settings.maxWateringCycles) {
          uint32_t minSec = _settings.minWateringInterval / 1000UL;
          uint32_t elapsedWater = nowUnix - _settings.lastWateringStartUnix;

          if (elapsedWater >= minSec) {
            if (!isnan(mat.humidity) && mat.humidity > _settings.matMinHumidity) {
              if (!isnan(mat.ec) && mat.ec < _settings.matMaxEC) {
                bool radCondition = (_settings.radSum >= (_settings.radThreshold * 1e6f));
                uint32_t maxSec = _settings.maxWateringInterval / 1000UL;
                bool maxIntervalCondition = (elapsedWater >= maxSec);

                if (radCondition || maxIntervalCondition) {
                  _settings.lastWateringStartUnix = nowUnix;
                  _settings.lastManualWateringTimeUnix = nowUnix;
                  _settings.cycleCount++;
                  _settings.radSum = 0.0f;
                  _settings.lastSunTimeUnix = nowUnix;
                  _settings.pumpState = true;
                  _storage.saveSettings(_settings, radCondition ? "auto_watering_started" : "auto_watering_by_max_interval");
                  LOG("DEBUG AUTO-WATER: STARTED by %s! Cycle %d\n",
                      radCondition ? "radiation" : "max interval", _settings.cycleCount);
                }
              }
            }
          }
        }
      }
    }

    // Остановка полива по истечении времени
    uint32_t elapsedWater = nowUnix - _settings.lastWateringStartUnix;
    uint32_t pumpSec = _settings.pumpTime / 1000UL;
    if (_settings.pumpState && elapsedWater >= pumpSec) {
      _settings.pumpState = false;
      _storage.saveSettings(_settings, "auto_watering_ended");
      LOG("DEBUG AUTO-WATER: Auto watering ended after %u sec\n", elapsedWater);
    }
  }

  // ==========================================
  // РЕЖИМ 1: MANUAL WATERING (Полив по интервалу)
  // ==========================================
  if (_settings.wateringMode == 1 && !_settings.forceWateringActive &&
      !_settings.forceWateringOverride && !_settings.morningWateringActive) {

    // Периодическая проверка (каждую минуту)
    if (now - _lastManualPeriodicCheck >= 60000) {
      _lastManualPeriodicCheck = now;

      if (currentMin >= startMin && currentMin < endMin) {
        uint32_t elapsedManual = nowUnix - _settings.lastManualWateringTimeUnix;
        uint32_t intervalSec = _settings.manualWateringInterval / 1000UL;

        if (elapsedManual >= intervalSec && level >= 10) {
          _settings.lastManualWateringTimeUnix = nowUnix;
          _settings.lastWateringStartUnix = nowUnix;
          _settings.radSum = 0.0f;
          _settings.lastSunTimeUnix = nowUnix;
          _settings.pumpState = true;
          _settings.cycleCount++;
          _storage.saveSettings(_settings, "manual_watering_started");
          LOG("DEBUG MANUAL-WATER: Started periodic watering, Cycle %d\n", _settings.cycleCount);
        }
      }
    }

    // Остановка по длительности
    uint32_t manualSec = _settings.manualWateringDuration / 1000UL;
    if (_settings.pumpState && (nowUnix - _settings.lastWateringStartUnix) >= manualSec) {
      _settings.pumpState = false;
      _storage.saveSettings(_settings, "manual_watering_ended");
      LOG("DEBUG MANUAL-WATER: Manual watering ended after %u sec\n", manualSec);
    }
  }

  // ==========================================
  // Управление реле насоса
  // ==========================================
  _actuators.setPump(_settings.pumpState);

  /*
   * TODO: Реализовать полную логику:
   * - Morning watering (утренний полив с несколькими циклами)
   * - Forced watering (принудительный полив с hydro mix и fill)
   * - Управление трехходовым клапаном
   * - Управление клапаном наполнения
   * - Восстановление состояния после перезагрузки
   */
}

// Управление ограничением ветра
void GreenhouseController::updateWindRestriction(float wind, uint32_t now) {
  // Активация ограничения при превышении любого из порогов ветра
  if (wind > _settings.wind1 || wind > _settings.wind2) {
    _windLockStart = now;
    _windRestricted = true;
  }

  // Снятие ограничения через заданное время бездействия ветра
  if (_windLockStart && (now - _windLockStart > _settings.windLockMinutes * 60 * 1000UL)) {
    _windRestricted = false;
    _windLockStart = 0;
  }
}

// Управление окнами
void GreenhouseController::updateWindowControl(float temp, float hum, uint32_t now) {
  // Проверка изменения ограничения по ветру
  bool windChanged = (_windRestricted != _prevWindRestricted);
  _prevWindRestricted = _windRestricted;

  // Проверка необходимости пересчета позиции окна
  bool shouldRecalcWindow = false;

  if (!_settings.manualWindow && _sensorsValid) {
    // Проверка изменения температуры или влажности с учетом гистерезиса
    if (isnan(_lastAvgTemp) || isnan(_lastAvgHum) ||
        abs(temp - _lastAvgTemp) >= TEMP_HYSTERESIS ||
        abs(hum - _lastAvgHum) >= HUM_HYSTERESIS) {
      shouldRecalcWindow = true;
    }

    // Немедленный пересчет при изменении ветрового ограничения
    if (windChanged) {
      shouldRecalcWindow = true;
      Serial.printf("Wind restriction changed to %s - immediate window recalculation\n",
                    _windRestricted ? "true" : "false");
    }

    if (shouldRecalcWindow) {
      _lastAvgTemp = temp;
      _lastAvgHum = hum;

      // Расчет эффективных пределов с учетом ночного смещения
      float effectiveMinTemp = _settings.minTemp;
      float effectiveMaxTemp = _settings.maxTemp;

      if (isNight()) {
        effectiveMinTemp -= _settings.nightOffset;
        effectiveMaxTemp -= _settings.nightOffset;
      }

      // Расчет целевой позиции на основе температуры и влажности
      float tTarget = mapFloat(temp, effectiveMinTemp, effectiveMaxTemp, 0, 100);
      float hTarget = mapFloat(hum, _settings.minHum, _settings.maxHum, 100, 0);
      int target = max(tTarget, hTarget);

      // Если температура ниже минимальной и влажность требует меньшего открытия - закрыть окно
      if (temp < effectiveMinTemp && hTarget > tTarget) {
        target = 0;
      }

      // Применение ограничений по ветру
      float wind = 0;
      if (_qWind) xQueuePeek(_qWind, &wind, 0);

      if (_windRestricted) {
        if (wind > _settings.wind2 && temp < 27) {
          target = 0;  // Полностью закрыть при сильном ветре и невысокой температуре
        } else if (wind > _settings.wind2) {
          target = min(target, 5);  // Ограничить до 5% при сильном ветре
        } else if (wind > _settings.wind1) {
          target = min(target, 20);  // Ограничить до 20% при умеренном ветре
        }
      }

      // Ограничение диапазона
      target = constrain(target, 0, 100);

      // Запуск движения окна, если позиция изменилась
      int currentWindowPos = _actuators.getWindowPosition();
      if (currentWindowPos != target && !_actuators.isWindowMoving() &&
          (now - _lastWindowMoveCommand > MIN_WINDOW_STATE_CHANGE_INTERVAL)) {

        int delta = abs(target - currentWindowPos);
        uint32_t fullTime = (target > currentWindowPos) ? _settings.openTime : _settings.closeTime;
        uint32_t duration = (delta / 100.0f) * fullTime;
        String dir = (target > currentWindowPos) ? "up" : "down";

        // Добавление дополнительного времени для полного закрытия
        if (dir == "down" && target == 0) {
          duration += 5000;
          Serial.println("Adding 5 sec extra for full window close");
        }

        _actuators.startWindowMovement(dir, duration, now);
        _actuators.setWindowPosition(target);  // Установка целевой позиции
        _lastWindowMoveCommand = now;
      }
    }
  }
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
