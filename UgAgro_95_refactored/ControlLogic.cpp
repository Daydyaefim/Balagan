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
    _lastDay(-1), _lastRadCheck(0), _lastAvgTemp(NAN), _lastAvgHum(NAN),
    _fanStartTimestamp(0), _initialized(false), _forcedPumpStarted(false),
    _lastMqttCheck(0), _lastFillChange(0) {
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

// Вспомогательная функция для маппинга float значений
float GreenhouseController::mapFloat(float x, float inMin, float inMax, float outMin, float outMax) const {
  if (inMax == inMin) return outMin;
  return constrain((x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin, outMin, outMax);
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

// Инициализация состояния полива при старте
void GreenhouseController::initializeWateringState() {
  _forcedPumpStarted = false; // Сброс флага при init

  // Сброс состояния тумана если активно при старте
  if (_settings.fogState) {
    _actuators.setFogPump(false);
    _actuators.setFogValve(false);
    _fog = false;
    _fogValveOn = false;
    _settings.fogState = false;
    _storage.saveSettings(_settings, "fog_init");
  }
  _taskLastFog = 0;
  _taskFogStartTime = 0;
  _fogValveStartTime = 0;

  DateTime nowDt = _rtc.now();
  uint32_t nowUnix = nowDt.unixtime();
  int currentDay = nowDt.day();
  int h = nowDt.hour(), m = nowDt.minute();
  int currentMin = h * 60 + m;
  int startMin = _settings.wateringStartHour * 60 + _settings.wateringStartMinute;
  int endMin = _settings.wateringEndHour * 60 + _settings.wateringEndMinute;

  float level = 0;
  if (_qLevel) xQueuePeek(_qLevel, &level, 0);

  // Проверка нового дня и сброс счетчиков
  if (currentDay != _lastDay) {
    _settings.cycleCount = 0;
    // НЕ сбрасываем radSum при новом дне для morning
    _settings.morningStartedToday = false;
    _settings.morningWateringActive = false;
    _settings.currentMorningWatering = 0;
    _settings.lastMorningWateringEndUnix = 0;
    _lastDay = currentDay;
    LOG("DEBUG AUTO-WATER: New day started. Reset cycleCount=%d, morningStartedToday=false\n", _settings.cycleCount);
    _storage.saveSettings(_settings, "new_day_reset");
  }

  // Восстановление morning watering после перезагрузки
  if (_settings.morningWateringActive && _settings.currentMorningWatering > 0 && _settings.morningWateringCount > 0) {
    uint32_t dayStartEstimate = nowUnix - (nowUnix % 86400);
    if (_settings.lastMorningWateringStartUnix >= (dayStartEstimate - 86400)) {
      uint32_t elapsedMorning = nowUnix - _settings.lastMorningWateringStartUnix;
      uint32_t durationSec = _settings.morningWateringDuration / 1000UL;
      if (elapsedMorning < durationSec) {
        _settings.pumpState = true;
        LOG("DEBUG MORNING-RESTORE: Restored morning #%d (%.1f sec left)\n", _settings.currentMorningWatering, durationSec - elapsedMorning);
      } else if (elapsedMorning >= (_settings.morningWateringInterval / 1000UL)) {
        if (_settings.currentMorningWatering < _settings.morningWateringCount) {
          _settings.currentMorningWatering++;
          _settings.lastMorningWateringStartUnix = nowUnix;
          _settings.pumpState = true;
          LOG("DEBUG MORNING-RESTORE: Started next morning #%d after restart\n", _settings.currentMorningWatering);
        } else {
          _settings.morningWateringActive = false;
          // НЕ сбрасываем radSum; lastWateringStartUnix на lastMorningStart
          _settings.lastWateringStartUnix = _settings.lastMorningWateringStartUnix;
          LOG("DEBUG MORNING-RESTORE: Sequence completed, morning inactive, lastWateringStart=lastMorningStart\n");
        }
      } else {
        _settings.pumpState = false;
        LOG("DEBUG MORNING-RESTORE: Morning interval not elapsed yet (%.1f sec left)\n", (_settings.morningWateringInterval / 1000UL) - elapsedMorning);
      }
    } else {
      _settings.morningWateringActive = false;
      _settings.morningStartedToday = false;
      _settings.currentMorningWatering = 0;
      LOG("DEBUG MORNING-RESTORE: Old day detected - reset morning state\n");
    }
    _storage.saveSettings(_settings, "morning_restore");
  }

  // Отмена morning если вне временного окна
  if (_settings.morningWateringActive && currentMin >= endMin) {
    _settings.morningWateringActive = false;
    _settings.morningStartedToday = true;
    _settings.currentMorningWatering = _settings.morningWateringCount;
    _settings.lastWateringStartUnix = _settings.lastMorningWateringStartUnix;
    LOG("DEBUG MORNING-RESTORE: Outside end time - cancel morning sequence, lastWateringStart=lastMorningStart\n");
    _storage.saveSettings(_settings, "morning_restore_endtime");
  }

  // Восстановление auto/manual watering после перезагрузки
  if ((_settings.wateringMode == 0 || _settings.wateringMode == 1) && currentMin >= startMin && currentMin < endMin) {
    uint32_t elapsedWater = nowUnix - _settings.lastWateringStartUnix;
    uint32_t durationSec = (_settings.wateringMode == 0 ? _settings.pumpTime : _settings.manualWateringDuration) / 1000UL;
    if (_settings.pumpState && elapsedWater < durationSec) {
      LOG("DEBUG WATER-RESTORE: Restored %s watering (%.1f sec left)\n", (_settings.wateringMode == 0 ? "auto" : "manual"), durationSec - elapsedWater);
    } else {
      _settings.pumpState = false;
    }
  }

  // Восстановление forced watering mode
  if (_settings.forceWateringActive) {
    if (nowUnix >= _settings.forceWateringEndTimeUnix) {
      _settings.forceWateringActive = false;
      _settings.pumpState = false;
      _settings.fillState = false;
      _settings.hydroMix = false;
      _settings.wateringMode = _settings.prevWateringMode;
      _settings.hydroStartUnix = 0;
      _settings.forceWateringEndTimeUnix = 0;

      // Если был реальный полив, сбросить таймеры здесь
      if (_settings.forcedWateringPerformed) {
        _settings.lastWateringStartUnix = nowUnix;
        int targetMode = _settings.prevWateringMode;
        if (targetMode == 0) {
          _settings.radSum = 0.0f;
          _settings.cycleCount++;
        } else if (targetMode == 1) {
          _settings.lastManualWateringTimeUnix = nowUnix;
        }
        _settings.forcedWateringPerformed = false;
        LOG("DEBUG FORCED-RESTORE: Forced timeout after restart - ended, timers reset (performed)\n");
      } else {
        LOG("DEBUG FORCED-RESTORE: Forced timeout after restart - ended, no reset (no pump)\n");
      }
      _storage.saveSettings(_settings, "forced_timeout_restore");
    } else {
      bool shouldPump = !(_settings.fillState && !_settings.hydroMix);
      if (shouldPump) {
        _settings.pumpState = true;
        _forcedPumpStarted = true;
        _settings.forcedWateringPerformed = true;
      } else {
        _settings.pumpState = false;
      }

      // Обработка hydroMix
      if (_settings.hydroMix) {
        uint32_t elapsedHydro = nowUnix - _settings.hydroStartUnix;
        uint32_t hydroSec = _settings.hydroMixDuration / 1000UL;
        if (elapsedHydro >= hydroSec) {
          _settings.hydroMix = false;
          _settings.hydroStartUnix = 0;
          LOG("DEBUG HYDRO-RESTORE: Hydro timeout after restart - ended\n");
          _storage.saveSettings(_settings, "hydro_restore_timeout");
        }
      }

      // Обработка fillState
      if (_settings.fillState && level >= 99.0) {
        _settings.fillState = false;
        LOG("DEBUG FILL-RESTORE: Fill complete after restart - ended\n");
        _storage.saveSettings(_settings, "fill_restore_complete");
      }

      LOG("DEBUG FORCED-RESTORE: Restored forced (%.1f sec left)\n", _settings.forceWateringEndTimeUnix - nowUnix);
      _storage.saveSettings(_settings, "forced_restore");
    }
  } else if (_settings.forceWateringOverride) {
    _settings.pumpState = false;
    LOG("DEBUG OVERRIDE-RESTORE: Override active after restart - pump OFF\n");
  }

  _initialized = true;
  Serial.println("GreenhouseController: Watering state initialization completed");
}

// Проверка ручных режимов
void GreenhouseController::checkManualModes(uint32_t now) {
  // TODO: Добавить проверки ручных режимов из оригинального кода
}

// Управление отоплением
void GreenhouseController::updateHeating(float temp, float sol) {
  uint32_t now = millis();

  // Автоматический режим
  if (!_settings.manualHeat && _sensorsValid && now - _lastHeatChange > MIN_STATE_CHANGE_INTERVAL) {
    float on = _settings.heatOn;
    float off = _settings.heatOff;

    // Применить ночное смещение если сейчас ночь
    if (isNight()) {
      on -= _settings.nightOffset;
      off -= _settings.nightOffset;
    }

    bool newHeatState = _heat;

    // Включить отопление если температура <= порога включения
    if (temp <= on && !_heat) {
      newHeatState = true;
      Serial.println("Heating turned ON");
      _lastHeatChange = now;
    }
    // Выключить отопление если температура >= порога выключения
    else if (temp >= off && _heat) {
      newHeatState = false;
      Serial.println("Heating turned OFF");
      _lastHeatChange = now;
    }

    // Сохранить состояние если изменилось
    if (_settings.heatState != newHeatState) {
      _settings.heatState = newHeatState;
      _heat = newHeatState;
      _storage.saveSettings(_settings, "heating");
    }

    _actuators.setHeating(newHeatState);
  }
  // Выключить отопление если датчики невалидны
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
    bool shouldTurnFanOnForFog = false;
    bool shouldTurnFanOnBySchedule = false;
    bool shouldTurnFanOffByDuration = false;

    // Включить вентилятор если включен туман
    if (!_fan && _fog) {
      shouldTurnFanOnForFog = true;
    }

    // Включить вентилятор по расписанию (если туман не активен)
    if (!_fan && !_fog) {
      if (now - _lastFanChange >= _settings.fanInterval) {
        shouldTurnFanOnBySchedule = true;
      }
    }

    // Выключить вентилятор по истечении времени работы
    if (_fan) {
      if (now - _fanStartTimestamp >= _settings.fanDuration) {
        shouldTurnFanOffByDuration = true;
      }
    }

    // Применить логику
    if (shouldTurnFanOnForFog) {
      targetFan = true;
    } else if (shouldTurnFanOffByDuration) {
      targetFan = false;
    } else if (shouldTurnFanOnBySchedule) {
      targetFan = true;
    }
  }

  // Применить изменение если состояние изменилось
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
      LOG("DEBUG: Fan state changed to %s. lastFanChange updated to %lu\n", _fan ? "ON" : "OFF", _lastFanChange);
    }
  }
}

// Управление нагревом раствора
void GreenhouseController::updateSolutionHeating(float sol) {
  uint32_t now = millis();
  float level = 0;
  if (_qLevel) xQueuePeek(_qLevel, &level, 0);

  // Автоматический режим (только если level > 20 и датчики валидны)
  if (!_settings.manualSol && level > 20 && _sensorsValid && now - _lastSolHeatChange > MIN_STATE_CHANGE_INTERVAL) {
    float on = _settings.solOn;
    float off = _settings.solOff;

    // Применить ночное смещение если сейчас ночь
    if (isNight()) {
      on -= _settings.nightOffset;
      off -= _settings.nightOffset;
    }

    bool newSolHeatState = _solHeat;

    // Включить нагрев если температура <= порога включения
    if (sol <= on && !_solHeat) {
      newSolHeatState = true;
      Serial.println("Solution heating turned ON");
      _lastSolHeatChange = now;
    }
    // Выключить нагрев если температура >= порога выключения
    else if (sol >= off && _solHeat) {
      newSolHeatState = false;
      Serial.println("Solution heating turned OFF");
      _lastSolHeatChange = now;
    }

    // Сохранить состояние если изменилось
    if (_settings.solHeatState != newSolHeatState) {
      _settings.solHeatState = newSolHeatState;
      _solHeat = newSolHeatState;
      _storage.saveSettings(_settings, "solution_heating");
    }

    _actuators.setSolutionHeating(newSolHeatState);
  }
  // Выключить нагрев если датчики невалидны
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

  bool morning = h >= _settings.fogMorningStart && h < _settings.fogMorningEnd;
  bool day = h >= _settings.autoFogDayStart && h < _settings.autoFogDayEnd;

  uint32_t dur = 0, intv = 0;
  bool shouldRun = false;

  // ===========================
  // РЕЖИМ FORCED (fogMode == 2)
  // ===========================
  if (_settings.fogMode == 2) {
    if (_settings.forceFogOn && !_fog) {
      // Сначала включить клапан, затем с задержкой насос
      if (!_fogValveOn) {
        _actuators.setFogValve(true);
        _fogValveOn = true;
        _fogValveStartTime = now;
        Serial.println("Fog valve turned ON in forced mode");
      } else if (now - _fogValveStartTime >= _settings.fogDelay * 1000UL) {
        _actuators.setFogPump(true);
        _fog = true;
        _fogValveOn = false;
        if (_settings.fogState != true) {
          _settings.fogState = true;
          _storage.saveSettings(_settings, "fog_forced");
        }
      }
    } else if (!_settings.forceFogOn && _fog) {
      // Выключить насос, подождать, выключить клапан
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
  // ===========================
  // РЕЖИМ MANUAL (fogMode == 1)
  // ===========================
  else if (_settings.fogMode == 1) {
    // Определить длительность и интервал в зависимости от времени суток
    if (morning) {
      dur = _settings.fogMorningDuration;
      intv = _settings.fogMorningInterval;
    } else if (day) {
      dur = _settings.fogDayDuration;
      intv = _settings.fogDayInterval;
    }

    // Включить туман если в пределах времени и интервал прошел
    if ((morning || day) && now - _taskLastFog > intv && !_fog) {
      if (!_fogValveOn) {
        _actuators.setFogValve(true);
        _fogValveOn = true;
        _fogValveStartTime = now;
        Serial.println("Fog valve turned ON in manual mode");
      } else if (now - _fogValveStartTime >= _settings.fogDelay * 1000UL) {
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

    // Выключить туман если длительность истекла
    if (_fog && now - _taskFogStartTime > dur) {
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
  // ===========================
  // РЕЖИМ AUTO (fogMode == 0)
  // ===========================
  else if (_settings.fogMode == 0) {
    // Утренний режим: фиксированные параметры
    if (morning) {
      dur = _settings.fogMorningDuration;
      intv = _settings.fogMorningInterval;
      shouldRun = true;
    }
    // Дневной режим: параметры зависят от температуры и влажности
    else if (day) {
      float fogHum = hum, fogTemp = temp;
      if (isnan(fogHum) || isnan(fogTemp)) {
        fogHum = _settings.fogMinHum + 1;
        fogTemp = (_settings.fogMinTemp + _settings.fogMaxTemp) / 2;
        Serial.println("Warning: Invalid avgHum or avgTemp, using defaults");
      }
      dur = calcFogDuration(fogHum, fogTemp);
      intv = calcFogInterval(fogHum, fogTemp);
      shouldRun = (fogHum < _settings.fogMaxHum &&
                   fogTemp >= _settings.fogMinTemp &&
                   fogTemp <= _settings.fogMaxTemp);
    }

    // Включить туман если условия выполнены и интервал прошел
    if (shouldRun && now - _taskLastFog > intv && !_fog) {
      if (!_fogValveOn) {
        _actuators.setFogValve(true);
        _fogValveOn = true;
        _fogValveStartTime = now;
        Serial.println("Fog valve turned ON in auto mode");
      } else if (now - _fogValveStartTime >= _settings.fogDelay * 1000UL) {
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

    // Выключить туман если длительность истекла
    if (_fog && now - _taskFogStartTime > dur) {
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

  // ===========================
  // ВЫКЛЮЧЕНИЕ ВНЕ ВРЕМЕННОГО ОКНА
  // ===========================
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
  // Инициализация при первом вызове
  if (!_initialized) {
    initializeWateringState();
  }

  DateTime nowTime = _rtc.now();
  uint32_t nowUnix = nowTime.unixtime();
  int h = nowTime.hour();
  int m = nowTime.minute();
  int currentMin = h * 60 + m;
  int startMin = _settings.wateringStartHour * 60 + _settings.wateringStartMinute;
  int endMin = _settings.wateringEndHour * 60 + _settings.wateringEndMinute;
  int currentDay = nowTime.day();

  float level = 0;
  if (_qLevel) xQueuePeek(_qLevel, &level, 0);

  // ===========================
  // ПРОВЕРКА НОВОГО ДНЯ
  // ===========================
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

  // ===========================
  // УТРЕННИЙ ПОЛИВ (MORNING WATERING)
  // ===========================
  if ((_settings.wateringMode == 0 || _settings.wateringMode == 1) && _settings.morningWateringCount > 0 &&
      !_settings.forceWateringActive && !_settings.forceWateringOverride) {
    uint32_t morningWindowMin = (_settings.morningWateringCount > 0) ?
      (_settings.morningWateringCount - 1UL) * (_settings.morningWateringInterval / 60000UL) : 0;

    // Проверка запуска утреннего полива
    if (!_settings.morningStartedToday && currentMin >= startMin && currentMin < endMin &&
        currentMin < (startMin + morningWindowMin)) {
      _settings.morningStartedToday = true;
      _settings.morningWateringActive = true;
      _settings.currentMorningWatering = 1;
      _settings.lastMorningWateringStartUnix = nowUnix;
      _settings.pumpState = true;
      _settings.lastWateringStartUnix = nowUnix; // Для morning используем текущий
      // НЕ сбрасываем radSum
      _settings.lastManualWateringTimeUnix = nowUnix;
      _settings.lastSunTimeUnix = nowUnix;
      _settings.cycleCount++;
      _storage.saveSettings(_settings, "morning_watering_started");
      LOG("DEBUG MORNING-WATER: Started sequence! Count=%u, Interval=%u ms, Duration=%u ms, Cycle %d (unix %lu)\n",
          _settings.morningWateringCount, _settings.morningWateringInterval, _settings.morningWateringDuration,
          _settings.cycleCount, nowUnix);
    }

    // Управление активным утренним поливом
    if (_settings.morningWateringActive) {
      uint32_t elapsedMorning = nowUnix - _settings.lastMorningWateringStartUnix;
      uint32_t durationSec = _settings.morningWateringDuration / 1000UL;

      // Выключить насос по истечении длительности
      if (_settings.pumpState && elapsedMorning >= durationSec) {
        _settings.pumpState = false;
        _settings.lastMorningWateringEndUnix = nowUnix;
        LOG("DEBUG MORNING-WATER: Ended watering #%u after %u sec (unix %lu)\n",
            _settings.currentMorningWatering, elapsedMorning, nowUnix);

        // Проверка завершения всей последовательности
        if (_settings.currentMorningWatering >= _settings.morningWateringCount) {
          _settings.morningWateringActive = false;
          // НЕ сбрасываем radSum; lastWateringStartUnix уже на старте последнего
          if (_settings.wateringMode == 1) {
            _settings.lastManualWateringTimeUnix = _settings.lastMorningWateringStartUnix;
          }
          _storage.saveSettings(_settings, "morning_watering_completed");
          LOG("DEBUG MORNING-WATER: Sequence completed! lastWateringStart remains lastMorningStart (unix %lu)\n",
              _settings.lastWateringStartUnix);

          // Проверка на немедленный запуск auto после morning, если radSum >= threshold
          if (_settings.wateringMode == 0 && _settings.radSum >= (_settings.radThreshold * 1e6)) {
            uint32_t minSec = _settings.minWateringInterval / 1000UL;
            uint32_t elapsedFromLastMorning = nowUnix - _settings.lastMorningWateringStartUnix;
            if (elapsedFromLastMorning >= minSec) {
              _settings.lastWateringStartUnix = nowUnix;
              _settings.cycleCount++;
              _settings.radSum = 0.0f;
              _settings.lastSunTimeUnix = nowUnix;
              _settings.pumpState = true;
              _storage.saveSettings(_settings, "auto_after_morning_rad");
              LOG("DEBUG AUTO-WATER: STARTED after morning by radiation! Cycle %d (unix %lu)\n",
                  _settings.cycleCount, nowUnix);
            }
          }
        }
      }

      // Запуск следующего полива в последовательности
      uint32_t intervalSec = _settings.morningWateringInterval / 1000UL;
      if (!_settings.pumpState && _settings.currentMorningWatering < _settings.morningWateringCount &&
          (nowUnix - _settings.lastMorningWateringStartUnix) >= intervalSec && currentMin < endMin) {
        _settings.currentMorningWatering++;
        _settings.lastMorningWateringStartUnix = nowUnix;
        _settings.pumpState = true;
        _settings.lastWateringStartUnix = nowUnix;
        // НЕ сбрасываем radSum
        _settings.lastManualWateringTimeUnix = nowUnix;
        _settings.lastSunTimeUnix = nowUnix;
        _settings.cycleCount++;
        _storage.saveSettings(_settings, "morning_watering_next");
        LOG("DEBUG MORNING-WATER: Started next watering #%u, Cycle %d (unix %lu)\n",
            _settings.currentMorningWatering, _settings.cycleCount, nowUnix);
      }
    }
  }

  // ===========================
  // АВТОМАТИЧЕСКИЙ ПОЛИВ (AUTO WATERING - wateringMode == 0)
  // ===========================
  if (_settings.wateringMode == 0 && !_settings.forceWateringActive &&
      !_settings.forceWateringOverride && !_settings.morningWateringActive) {
    static uint32_t lastAutoWateringCheck = 0;

    // Периодическая проверка по интервалу radCheckInterval
    if (millis() - lastAutoWateringCheck >= _settings.radCheckInterval) {
      lastAutoWateringCheck = millis();
      LOG("DEBUG AUTO-WATER: Auto watering check interval elapsed. Current radSum=%.2f J/m2, threshold=%.2f MJ/m2 (%.2f J/m2)\n",
          _settings.radSum, _settings.radThreshold, _settings.radThreshold * 1e6);

      // Накопление солнечной радиации
      if (!isnan(pyrano) && pyrano > 0) {
        float deltaRad = pyrano * (_settings.radCheckInterval / 1000.0);
        _settings.radSum += deltaRad;
        _settings.lastSunTimeUnix = nowUnix;
        LOG("DEBUG AUTO-WATER: Radiation accumulation: pyrano=%.2f W/m2, delta=%.2f J/m2, new radSum=%.2f J/m2 (unix %lu)\n",
            pyrano, deltaRad, _settings.radSum, nowUnix);
      } else {
        LOG("DEBUG AUTO-WATER: No radiation detected (pyrano=%.2f), checking reset\n", pyrano);
        // Сброс radSum если нет солнца долгое время
        uint32_t resetSec = _settings.radSumResetInterval / 1000UL;
        if ((nowUnix - _settings.lastSunTimeUnix) >= resetSec) {
          _settings.radSum = 0.0f;
          _settings.lastSunTimeUnix = nowUnix;
          LOG("DEBUG AUTO-WATER: radSum reset to 0 due to no sun for %u sec (unix %lu)\n", resetSec, nowUnix);
        }
      }

      // Проверка условий запуска полива
      if (currentMin >= startMin && currentMin < endMin) {
        LOG("DEBUG AUTO-WATER: Within watering hours (%02d:%02d - %02d:%02d). Checking conditions...\n",
            _settings.wateringStartHour, _settings.wateringStartMinute,
            _settings.wateringEndHour, _settings.wateringEndMinute);

        if (level >= 2) {
          LOG("DEBUG AUTO-WATER: Level OK (%.1f%% >=2). Cycles left: %d/%d\n",
              level, _settings.maxWateringCycles - _settings.cycleCount, _settings.maxWateringCycles);

          if (_settings.cycleCount < _settings.maxWateringCycles) {
            uint32_t minSec = _settings.minWateringInterval / 1000UL;
            uint32_t elapsedWater = nowUnix - _settings.lastWateringStartUnix;

            if (elapsedWater >= minSec) {
              LOG("DEBUG AUTO-WATER: Min interval OK (last watering %u sec ago >= %u sec)\n", elapsedWater, minSec);

              // Проверка mat sensor
              if (!isnan(mat.humidity) && mat.humidity > _settings.matMinHumidity) {
                LOG("DEBUG AUTO-WATER: Mat humidity OK (%.1f%% > %.1f%%)\n", mat.humidity, _settings.matMinHumidity);

                if (!isnan(mat.ec) && mat.ec < _settings.matMaxEC) {
                  LOG("DEBUG AUTO-WATER: Mat EC OK (%.2f < %.2f dS/m)\n", mat.ec, _settings.matMaxEC);

                  // Проверка условий запуска: по радиации или по максимальному интервалу
                  bool radCondition = (_settings.radSum >= (_settings.radThreshold * 1e6));
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
                    LOG("DEBUG AUTO-WATER: STARTED by %s! Cycle %d, radSum reset (unix %lu)\n",
                        radCondition ? "radiation" : "max interval", _settings.cycleCount, nowUnix);
                  } else if (mat.humidity < (_settings.matMinHumidity - 10.0)) {
                    // Особый случай: очень низкая влажность субстрата
                    _settings.lastWateringStartUnix = nowUnix;
                    _settings.lastManualWateringTimeUnix = nowUnix;
                    _settings.cycleCount++;
                    _settings.radSum = 0.0f;
                    _settings.lastSunTimeUnix = nowUnix;
                    _settings.pumpState = true;
                    _storage.saveSettings(_settings, "extra_watering_started");
                    LOG("DEBUG AUTO-WATER: STARTED by low mat humidity (%.1f%% < %.1f%%)! Cycle %d (unix %lu)\n",
                        mat.humidity, _settings.matMinHumidity - 10.0, _settings.cycleCount, nowUnix);
                  } else {
                    LOG("DEBUG AUTO-WATER: Conditions not met - radSum=%.2f < %.2f J/m2 (maxInt=%.2f h), matHum=%.1f%% >= %.1f%%\n",
                        _settings.radSum, _settings.radThreshold * 1e6, (float)elapsedWater / 3600.0,
                        mat.humidity, _settings.matMinHumidity);
                  }
                } else {
                  LOG("DEBUG AUTO-WATER: Mat EC high (%.2f >= %.2f) - no watering\n", mat.ec, _settings.matMaxEC);
                }
              } else {
                LOG("DEBUG AUTO-WATER: Mat humidity low or invalid (%.1f%% <= %.1f%%) - no watering\n",
                    mat.humidity, _settings.matMinHumidity);
              }
            } else {
              LOG("DEBUG AUTO-WATER: Min interval not elapsed (%u sec < %u sec) - skip\n", elapsedWater, minSec);
            }
          } else {
            LOG("DEBUG AUTO-WATER: Max cycles reached (%d/%d) - skip\n",
                _settings.cycleCount, _settings.maxWateringCycles);
          }
        } else {
          LOG("DEBUG AUTO-WATER: Low level (%.1f%% <2) - skip\n", level);
        }
      } else {
        LOG("DEBUG AUTO-WATER: Outside watering hours (current %02d:%02d) - skip\n", h, m);
      }
    }

    // Выключение насоса по истечении pumpTime
    uint32_t elapsedWater = nowUnix - _settings.lastWateringStartUnix;
    uint32_t pumpSec = _settings.pumpTime / 1000UL;
    if (_settings.pumpState && elapsedWater >= pumpSec) {
      _settings.pumpState = false;
      _storage.saveSettings(_settings, "auto_watering_ended");
      LOG("DEBUG AUTO-WATER: Auto watering ended after %u sec (unix %lu)\n", elapsedWater, nowUnix);
    }
  }

  // ===========================
  // МАНУАЛЬНЫЙ ПОЛИВ (MANUAL WATERING - wateringMode == 1)
  // ===========================
  if (_settings.wateringMode == 1 && !_settings.forceWateringActive &&
      !_settings.forceWateringOverride && !_settings.morningWateringActive) {
    static uint32_t lastManualPeriodicCheck = 0;

    // Периодическая проверка каждые 60 секунд
    if (millis() - lastManualPeriodicCheck >= 60000) {
      lastManualPeriodicCheck = millis();

      // Проверка в пределах временного окна полива
      if (currentMin >= startMin && currentMin < endMin) {
        uint32_t elapsedManual = nowUnix - _settings.lastManualWateringTimeUnix;
        uint32_t intervalSec = _settings.manualWateringInterval / 1000UL;

        // Запуск полива если прошел интервал и уровень достаточный
        if (elapsedManual >= intervalSec && level >= 10) {
          _settings.lastManualWateringTimeUnix = nowUnix;
          _settings.lastWateringStartUnix = nowUnix;
          _settings.radSum = 0.0f;
          _settings.lastSunTimeUnix = nowUnix;
          _settings.pumpState = true;
          _settings.cycleCount++;
          _storage.saveSettings(_settings, "manual_watering_started");
          LOG("DEBUG MANUAL-WATER: Started periodic watering, Cycle %d (unix %lu)\n",
              _settings.cycleCount, nowUnix);
        }
      }
    }

    // Выключение насоса по истечении manualWateringDuration
    uint32_t manualSec = _settings.manualWateringDuration / 1000UL;
    if (_settings.pumpState && (nowUnix - _settings.lastWateringStartUnix) >= manualSec) {
      _settings.pumpState = false;
      _storage.saveSettings(_settings, "manual_watering_ended");
      LOG("DEBUG MANUAL-WATER: Manual watering ended after %u sec (unix %lu)\n", manualSec, nowUnix);
    }
  }

  // ===========================
  // FORCED РЕЖИМ ПОЛИВА
  // ===========================
  if (_settings.forceWateringActive) {
    bool targetPumpState = false;
    bool targetThreeWayState = false;
    bool targetFillState = _settings.fillState;
    bool shouldTransitionToNormal = false;
    int transitionMode = _settings.prevWateringMode; // По умолчанию к предыдущему

    // Обработка hydroMix (смешивание раствора)
    if (_settings.hydroMix) {
      uint32_t hydroSec = _settings.hydroMixDuration / 1000UL;
      if ((nowUnix - _settings.hydroStartUnix) < hydroSec) {
        targetPumpState = true;
        targetThreeWayState = true;
        if (!_forcedPumpStarted) {
          _forcedPumpStarted = true;
          _settings.forcedWateringPerformed = true;
          _settings.lastWateringStartUnix = nowUnix;
          LOG("DEBUG: Hydro mix active - REL_PUMP HIGH, REL_3WAY HIGH, timers reset\n");
        } else {
          LOG("DEBUG: Hydro mix active - REL_PUMP HIGH, REL_3WAY HIGH\n");
        }
      } else {
        _settings.hydroMix = false;
        _settings.hydroStartUnix = 0;
        LOG("DEBUG: Hydro mix timeout - continue fill if active\n");
        if (!_settings.fillState) {
          // Если нет fill, но был hydro — переходить, если таймаут основной истёк
          if (nowUnix >= _settings.forceWateringEndTimeUnix) {
            shouldTransitionToNormal = true;
          }
        } // Иначе продолжить fill
      }
    }

    // Обработка fillState (наполнение бака)
    if (_settings.fillState) {
      if (level >= 99.0) {
        _settings.fillState = false;
        LOG("DEBUG: Fill complete - check for transition\n");
        // Если был hydro (даже завершённый), теперь переходить
        if (!_settings.hydroMix) { // Hydro уже завершён или не был
          shouldTransitionToNormal = true;
        }
        // Если hydro ещё идёт, продолжить (но hydro таймаут выше)
      } else {
        targetFillState = true;
        // Для fill без hydro: pump только если нужно, но без сброса таймера (fill не считается поливом)
        targetPumpState = _settings.hydroMix; // Насос только если hydro
        targetThreeWayState = _settings.hydroMix;
        LOG("DEBUG: Fill active - REL_FILL HIGH (level %.1f%%)\n", level);
      }
    } else {
      // Нет fill и нет hydro — стандартный таймаут
      targetPumpState = (nowUnix < _settings.forceWateringEndTimeUnix);
      if (targetPumpState && !_forcedPumpStarted) {
        _forcedPumpStarted = true;
        _settings.forcedWateringPerformed = true;
        _settings.lastWateringStartUnix = nowUnix;
        LOG("DEBUG: Forced pump start - timers reset\n");
      }
      if (!targetPumpState && !_forcedPumpStarted && nowUnix >= _settings.forceWateringEndTimeUnix) {
        // Если не стартовал pump, не засчитывать
        shouldTransitionToNormal = true;
      }
    }

    // Переход к нормальному режиму
    if (shouldTransitionToNormal) {
      int targetMode = _settings.prevWateringMode;
      if (!_settings.forcedWateringPerformed) {
        targetMode = 2; // Переход в режим 2 (выключено) если не было реального полива
      }

      _actuators.setPump(false);
      _settings.pumpState = false;
      _settings.forceWateringActive = false;
      _settings.wateringMode = targetMode;

      // Сброс таймеров только если был performed
      if (_settings.forcedWateringPerformed) {
        _settings.lastWateringStartUnix = nowUnix;
        if (targetMode == 0) {
          _settings.radSum = 0.0f;
          _settings.cycleCount++;
        } else if (targetMode == 1) {
          _settings.lastManualWateringTimeUnix = nowUnix;
        }
        _settings.forcedWateringPerformed = false;
      }

      _settings.forceWateringOverride = false;
      _settings.forceWateringEndTimeUnix = 0;

      // Обработка morning после forced
      if (targetMode == 0 || targetMode == 1) {
        if (_settings.morningWateringActive) {
          _settings.lastMorningWateringEndUnix = nowUnix;
          if (_settings.pendingMorningComplete) {
            _settings.currentMorningWatering++;
            _settings.pendingMorningComplete = false;
            LOG("DEBUG MORNING-WATER: Forced counted as completed morning watering #%u\n",
                _settings.currentMorningWatering);
          }

          // Проверка завершения morning последовательности
          if (_settings.currentMorningWatering >= _settings.morningWateringCount) {
            _settings.morningWateringActive = false;
            // НЕ сбрасываем radSum
            _settings.lastWateringStartUnix = _settings.lastMorningWateringStartUnix;
            if (targetMode == 1) {
              _settings.lastManualWateringTimeUnix = _settings.lastMorningWateringStartUnix;
            }
            _storage.saveSettings(_settings, "morning_completed_after_forced");
            LOG("DEBUG MORNING-WATER: Sequence completed after forced! lastWateringStart=lastMorningStart\n");

            // Проверка на auto после forced+morning
            if (targetMode == 0 && _settings.radSum >= (_settings.radThreshold * 1e6)) {
              uint32_t minSec = _settings.minWateringInterval / 1000UL;
              uint32_t elapsedFromLastMorning = nowUnix - _settings.lastMorningWateringStartUnix;
              if (elapsedFromLastMorning >= minSec) {
                _settings.lastWateringStartUnix = nowUnix;
                _settings.cycleCount++;
                _settings.radSum = 0.0f;
                _settings.lastSunTimeUnix = nowUnix;
                _settings.pumpState = true;
                _storage.saveSettings(_settings, "auto_after_forced_morning_rad");
                LOG("DEBUG AUTO-WATER: STARTED after forced+morning by radiation! Cycle %d (unix %lu)\n",
                    _settings.cycleCount, nowUnix);
              }
            }
          }
        }
      }

      _storage.saveSettings(_settings, "force_watering_transition");
      LOG("DEBUG: Force watering transition to mode %d after completion (unix %lu, performed=%s)\n",
          targetMode, nowUnix, _settings.forcedWateringPerformed ? "true" : "false");

      // Явная публикация после перехода (используя MQTT напрямую)
      if (_mqtt.isConnected()) {
        SensorData avgData;
        float sol = 0, wind = 0;
        SensorData outdoor;
        MatSensorData matData;

        if (_qAvg) xQueuePeek(_qAvg, &avgData, 0);
        if (_qSol) xQueuePeek(_qSol, &sol, 0);
        if (_qWind) xQueuePeek(_qWind, &wind, 0);
        if (_qOutdoor) xQueuePeek(_qOutdoor, &outdoor, 0);
        if (_qMat) xQueuePeek(_qMat, &matData, 0);

        _mqtt.publishAllSensors(avgData, sol, level, wind, outdoor, matData, pyrano,
                                 _actuators.getWindowPosition(), _settings);
      }
    } else {
      // Применить состояние актуаторов
      _actuators.setPump(targetPumpState);
      _actuators.setThreeWayValve(targetThreeWayState);
      _actuators.setFillValve(targetFillState);
    }
  }

  // ===========================
  // OVERRIDE РЕЖИМ
  // ===========================
  if (_settings.forceWateringOverride) {
    // Принудительное выключение всех систем полива
    _actuators.setPump(false);
    _actuators.setThreeWayValve(false);
    _actuators.setFillValve(false);
    LOG("DEBUG: Override active - all watering OFF\n");
  }

  // ===========================
  // ПРИМЕНЕНИЕ СОСТОЯНИЯ НАСОСА
  // ===========================
  // TODO: Синхронизация состояния актуаторов с _settings.pumpState
  // Текущая заглушка - просто применяем состояние
  static bool lastPumpState = false;
  if (_settings.pumpState != lastPumpState) {
    _actuators.setPump(_settings.pumpState);
    lastPumpState = _settings.pumpState;
    _storage.saveSettings(_settings, _settings.pumpState ? "pump_turned_on" : "pump_turned_off");
  }
}

// Управление ограничением ветра
void GreenhouseController::updateWindRestriction(float wind, uint32_t now) {
  // Активировать блокировку если ветер превышает пороги
  if (wind > _settings.wind1 || wind > _settings.wind2) {
    _windLockStart = now;
    _windRestricted = true;
  }

  // Снять блокировку после истечения времени блокировки
  if (_windLockStart && now - _windLockStart > _settings.windLockMinutes * 60 * 1000UL) {
    _windRestricted = false;
    _windLockStart = 0;
  }
}

// Управление окнами
void GreenhouseController::updateWindowControl(float temp, float hum, uint32_t now) {
  float wind = 0;
  if (_qWind) xQueuePeek(_qWind, &wind, 0);

  // Проверка изменения состояния ветра
  bool windChanged = (_windRestricted != _prevWindRestricted);
  _prevWindRestricted = _windRestricted;

  bool shouldRecalcWindow = false;

  // Автоматический режим управления окнами
  if (!_settings.manualWindow && _sensorsValid) {
    // Перерасчет если изменились temp/hum с учетом гистерезиса
    if (isnan(_lastAvgTemp) || isnan(_lastAvgHum) ||
        abs(temp - _lastAvgTemp) >= TEMP_HYSTERESIS ||
        abs(hum - _lastAvgHum) >= HUM_HYSTERESIS) {
      shouldRecalcWindow = true;
    }

    // Перерасчет если изменилось состояние ветра
    if (windChanged) {
      shouldRecalcWindow = true;
      Serial.printf("Wind restriction changed to %s - immediate window recalculation\n",
                    _windRestricted ? "true" : "false");
    }

    if (shouldRecalcWindow) {
      _lastAvgTemp = temp;
      _lastAvgHum = hum;

      // Применить ночное смещение если сейчас ночь
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

      // Особый случай: если температура ниже минимума и влажность требует большего открытия
      if (temp < effectiveMinTemp && hTarget > tTarget) {
        target = 0;
      }

      // Применить ограничения по ветру
      if (_windRestricted) {
        if (wind > _settings.wind2 && temp < 27) {
          target = 0;
        } else if (wind > _settings.wind2) {
          target = min(target, 5);
        } else if (wind > _settings.wind1) {
          target = min(target, 20);
        }
      }

      target = constrain(target, 0, 100);

      // Запустить движение окна если позиция изменилась
      int windowPos = _actuators.getWindowPosition();
      if (windowPos != target && !_actuators.isWindowMoving() &&
          now - _lastWindowMoveCommand > MIN_WINDOW_STATE_CHANGE_INTERVAL) {
        int delta = abs(target - windowPos);
        uint32_t fullTime = (target > windowPos) ? _settings.openTime : _settings.closeTime;
        uint32_t duration = (delta / 100.0f) * fullTime;
        String dir = (target > windowPos) ? "up" : "down";

        // Добавить дополнительное время для полного закрытия
        if (dir == "down" && target == 0) {
          duration += 5000;
          Serial.println("Добавляем 5 сек extra для полного закрытия форточки");
        }

        _actuators.startWindowMovement(dir, duration, now);
        _lastWindowMoveCommand = now;
      }
    }
  }

  // Проверка завершения автоматического движения окна
  if (_actuators.isWindowMoving() && !_settings.manualWindow) {
    // updateWindowPosition в ActuatorManager сам остановит движение когда время истечет
    // Публикация новой позиции в MQTT будет выполнена в update()
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
