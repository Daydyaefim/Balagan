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
  // TODO: Перенести полную логику инициализации из UgAgro_95.ino строки 2131-2275
  // - Сброс состояния тумана если активно
  // - Проверка нового дня и сброс счетчиков
  // - Восстановление morning watering после перезагрузки
  // - Восстановление auto/manual watering после перезагрузки
  // - Восстановление forced watering mode
  // - Обработка hydroMix и fillState

  _initialized = true;
  Serial.println("GreenhouseController: Watering state initialization completed (structure only)");
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
  // TODO: Полная реализация из строк 2296-2359
  // - Проверка morningWateringCount > 0
  // - Запуск последовательности утренних поливов
  // - Управление интервалами и длительностью
  // - Проверка немедленного auto полива после morning если radSum >= threshold

  // ===========================
  // АВТОМАТИЧЕСКИЙ ПОЛИВ (AUTO WATERING - wateringMode == 0)
  // ===========================
  if (_settings.wateringMode == 0 && !_settings.forceWateringActive &&
      !_settings.forceWateringOverride && !_settings.morningWateringActive) {
    // TODO: Полная реализация из строк 2360-2450
    // - Накопление солнечной радиации (radSum += pyrano * delta_time)
    // - Сброс radSum если нет солнца долгое время
    // - Проверка условий: в пределах часов полива, уровень >= 2%, циклы < max
    // - Проверка mat sensor: humidity > min, EC < max
    // - Запуск полива по радиации или максимальному интервалу
    // - Выключение по истечении pumpTime

    LOG("DEBUG: AUTO watering mode active (TODO: full implementation)\n");
  }

  // ===========================
  // МАНУАЛЬНЫЙ ПОЛИВ (MANUAL WATERING - wateringMode == 1)
  // ===========================
  if (_settings.wateringMode == 1 && !_settings.forceWateringActive &&
      !_settings.forceWateringOverride && !_settings.morningWateringActive) {
    // TODO: Полная реализация из строк 2452-2480
    // - Периодическая проверка (каждые 60 сек)
    // - Запуск полива по интервалу manualWateringInterval
    // - Длительность manualWateringDuration
    // - Проверка level >= 10%

    LOG("DEBUG: MANUAL watering mode active (TODO: full implementation)\n");
  }

  // ===========================
  // FORCED РЕЖИМ ПОЛИВА
  // ===========================
  if (_settings.forceWateringActive) {
    // TODO: Полная реализация из строк 2482-2606
    // - Управление hydroMix (3-way valve, pump)
    // - Управление fillState (fill valve)
    // - Проверка завершения forced режима
    // - Переход к нормальному режиму с сбросом таймеров
    // - Обработка forcedWateringPerformed флага

    LOG("DEBUG: FORCED watering mode active (TODO: full implementation)\n");
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
