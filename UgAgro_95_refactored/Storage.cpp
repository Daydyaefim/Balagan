// ===========================
// МОДУЛЬ ХРАНЕНИЯ ДАННЫХ - РЕАЛИЗАЦИЯ
// ===========================

#include "Storage.h"

// Константы путей к файлам
const char* StorageManager::SETTINGS_FILE = "/settings.json";
const char* StorageManager::HISTORY_FILE = "/history.json";

// Конструктор
StorageManager::StorageManager() : _lastSaveTime(0) {
}

// Инициализация файловой системы
bool StorageManager::begin() {
  if (!LittleFS.begin()) {
    LOG("StorageManager: Ошибка монтирования LittleFS\n");
    return false;
  }
  LOG("StorageManager: LittleFS смонтирована успешно\n");
  return true;
}

// Применение настроек по умолчанию
void StorageManager::applyDefaultSettings(Settings& settings) {
  LOG("StorageManager: Применение настроек по умолчанию\n");

  // Температура и влажность
  settings.minTemp = 20.0f;
  settings.maxTemp = 28.0f;
  settings.minHum = 50.0f;
  settings.maxHum = 90.0f;

  // Окна
  settings.openTime = 156000;  // 156 секунд
  settings.closeTime = 156000;
  settings.manualWindow = false;
  settings.windowCommand = "";
  settings.windowDuration = 0;

  // Отопление
  settings.heatOn = 18.0f;
  settings.heatOff = 22.0f;
  settings.manualHeat = false;
  settings.heatState = false;

  // Вентилятор
  settings.fanInterval = 30 * 60 * 1000UL; // 30 минут
  settings.fanDuration = 5 * 60 * 1000UL;  // 5 минут
  settings.manualFan = false;
  settings.fanState = false;

  // Туманообразование
  settings.fogMorningStart = 7;
  settings.fogMorningEnd = 8;
  settings.fogDayStart = 8;
  settings.fogDayEnd = 16;
  settings.fogMorningDuration = 10 * 1000UL;
  settings.fogMorningInterval = 30 * 60 * 1000UL;
  settings.fogDayDuration = 10 * 1000UL;
  settings.fogDayInterval = 2 * 60 * 1000UL;
  settings.fogDelay = 30 * 1000UL;
  settings.fogMinHum = 60.0f;
  settings.fogMaxHum = 95.0f;
  settings.fogMinTemp = 18.0f;
  settings.fogMaxTemp = 30.0f;
  settings.fogMinDuration = 5 * 1000UL;
  settings.fogMaxDuration = 30 * 1000UL;
  settings.fogMinInterval = 10 * 60 * 1000UL;
  settings.fogMaxInterval = 2 * 60 * 60 * 1000UL;
  settings.fogMode = 0;
  settings.forceFogOn = false;
  settings.autoFogDayStart = 10;
  settings.autoFogDayEnd = 17;
  settings.fogState = false;

  // Раствор
  settings.solOn = 18.0f;
  settings.solOff = 22.0f;
  settings.manualSol = false;
  settings.solHeatState = false;

  // Ветер
  settings.wind1 = 5.0f;
  settings.wind2 = 10.0f;
  settings.windLockMinutes = 5;

  // День/ночь
  settings.nightStart = 22;
  settings.dayStart = 6;
  settings.nightOffset = 2.0f;

  // Уровень
  settings.levelMin = 10.0f;
  settings.levelMax = 90.0f;
  settings.fillState = false;

  // Гидропоника
  settings.hydroMixDuration = 10 * 60 * 1000UL;
  settings.hydroMix = false;
  settings.hydroStart = 0;
  settings.hydroStartUnix = 0;

  // Полив
  settings.manualPump = false;
  settings.previousManualPump = false;
  settings.manualPumpOverride = false;
  settings.pumpState = false;
  settings.wateringMode = 0;
  settings.manualWateringInterval = 60 * 60 * 1000UL;
  settings.manualWateringDuration = 5 * 60 * 1000UL;
  settings.matMinHumidity = 60.0f;
  settings.matMaxEC = 2.0f;
  settings.radThreshold = 100.0f;
  settings.pumpTime = 0;
  settings.wateringStartHour = 8;
  settings.wateringStartMinute = 0;
  settings.wateringEndHour = 18;
  settings.wateringEndMinute = 0;
  settings.maxWateringCycles = 5;
  settings.radSumResetInterval = 24 * 60 * 60 * 1000UL;
  settings.minWateringInterval = 2 * 60 * 60 * 1000UL;
  settings.maxWateringInterval = 8 * 60 * 60 * 1000UL;
  settings.forcedWateringDuration = 5 * 60 * 1000UL;
  settings.prevWateringMode = -1;
  settings.morningWateringCount = 3;
  settings.morningWateringInterval = 15 * 60 * 1000UL;
  settings.morningWateringDuration = 3 * 60 * 1000UL;
  settings.radCheckInterval = 5 * 60 * 1000UL;
  settings.radSum = 0.0f;
  settings.cycleCount = 0;
  settings.lastWateringStartUnix = 0;
  settings.lastSunTimeUnix = 0;
  settings.lastManualWateringTimeUnix = 0;
  settings.morningStartedToday = false;
  settings.morningWateringActive = false;
  settings.currentMorningWatering = 0;
  settings.lastMorningWateringStartUnix = 0;
  settings.lastMorningWateringEndUnix = 0;
  settings.pendingMorningComplete = false;
  settings.forceWateringActive = false;
  settings.forceWateringOverride = false;
  settings.forceWateringEndTime = 0;
  settings.forceWateringEndTimeUnix = 0;
  settings.forcedWateringPerformed = false;

  // Режимы полива
  for (int i = 0; i < 4; i++) {
    settings.wateringModes[i].enabled = false;
    settings.wateringModes[i].startHour = 8;
    settings.wateringModes[i].endHour = 18;
    settings.wateringModes[i].duration = 5 * 60 * 1000UL;
  }
}

// Проверка, что настройки обнулены
bool StorageManager::isSettingsZeroed(const Settings& settings) {
  return (settings.minTemp == 0 && settings.maxTemp == 0 &&
          settings.openTime == 0 && settings.closeTime == 0);
}

// Сериализация настроек в JSON (УПРОЩЕННАЯ ВЕРСИЯ - показывает основные поля)
bool StorageManager::serializeSettings(const Settings& settings, DynamicJsonDocument& doc) {
  // Основные параметры
  doc["minTemp"] = settings.minTemp;
  doc["maxTemp"] = settings.maxTemp;
  doc["minHum"] = settings.minHum;
  doc["maxHum"] = settings.maxHum;
  doc["openTime"] = settings.openTime;
  doc["closeTime"] = settings.closeTime;
  doc["heatOn"] = settings.heatOn;
  doc["heatOff"] = settings.heatOff;
  doc["fanInterval"] = settings.fanInterval;
  doc["fanDuration"] = settings.fanDuration;

  // Туманообразование
  doc["fogMorningStart"] = settings.fogMorningStart;
  doc["fogMorningEnd"] = settings.fogMorningEnd;
  doc["fogDayStart"] = settings.fogDayStart;
  doc["fogDayEnd"] = settings.fogDayEnd;
  doc["fogMode"] = settings.fogMode;

  // Раствор
  doc["solOn"] = settings.solOn;
  doc["solOff"] = settings.solOff;

  // Ветер
  doc["wind1"] = settings.wind1;
  doc["wind2"] = settings.wind2;
  doc["windLockMinutes"] = settings.windLockMinutes;

  // День/ночь
  doc["nightStart"] = settings.nightStart;
  doc["dayStart"] = settings.dayStart;

  // Уровень
  doc["levelMin"] = settings.levelMin;
  doc["levelMax"] = settings.levelMax;

  // Полив
  doc["wateringMode"] = settings.wateringMode;
  doc["matMinHumidity"] = settings.matMinHumidity;
  doc["radThreshold"] = settings.radThreshold;

  // Режимы полива
  JsonArray wmArray = doc.createNestedArray("wateringModes");
  for (int i = 0; i < 4; i++) {
    JsonObject wm = wmArray.createNestedObject();
    wm["enabled"] = settings.wateringModes[i].enabled;
    wm["startHour"] = settings.wateringModes[i].startHour;
    wm["endHour"] = settings.wateringModes[i].endHour;
    wm["duration"] = settings.wateringModes[i].duration;
  }

  // ПРИМЕЧАНИЕ: В полной версии необходимо добавить все остальные поля из структуры Settings

  return true;
}

// Десериализация настроек из JSON (УПРОЩЕННАЯ ВЕРСИЯ)
bool StorageManager::deserializeSettings(Settings& settings, const DynamicJsonDocument& doc) {
  settings.minTemp = doc["minTemp"] | 20.0f;
  settings.maxTemp = doc["maxTemp"] | 28.0f;
  settings.minHum = doc["minHum"] | 50.0f;
  settings.maxHum = doc["maxHum"] | 90.0f;
  settings.openTime = doc["openTime"] | 156000;
  settings.closeTime = doc["closeTime"] | 156000;
  settings.heatOn = doc["heatOn"] | 18.0f;
  settings.heatOff = doc["heatOff"] | 22.0f;
  settings.fanInterval = doc["fanInterval"] | (30 * 60 * 1000UL);
  settings.fanDuration = doc["fanDuration"] | (5 * 60 * 1000UL);

  // Туманообразование
  settings.fogMorningStart = doc["fogMorningStart"] | 7;
  settings.fogMorningEnd = doc["fogMorningEnd"] | 8;
  settings.fogDayStart = doc["fogDayStart"] | 8;
  settings.fogDayEnd = doc["fogDayEnd"] | 16;
  settings.fogMode = doc["fogMode"] | 0;

  // Раствор
  settings.solOn = doc["solOn"] | 18.0f;
  settings.solOff = doc["solOff"] | 22.0f;

  // Ветер
  settings.wind1 = doc["wind1"] | 5.0f;
  settings.wind2 = doc["wind2"] | 10.0f;
  settings.windLockMinutes = doc["windLockMinutes"] | 5;

  // День/ночь
  settings.nightStart = doc["nightStart"] | 22;
  settings.dayStart = doc["dayStart"] | 6;

  // Уровень
  settings.levelMin = doc["levelMin"] | 10.0f;
  settings.levelMax = doc["levelMax"] | 90.0f;

  // Полив
  settings.wateringMode = doc["wateringMode"] | 0;
  settings.matMinHumidity = doc["matMinHumidity"] | 60.0f;
  settings.radThreshold = doc["radThreshold"] | 100.0f;

  // Режимы полива
  JsonArray wmArray = doc["wateringModes"];
  if (wmArray.size() == 4) {
    for (int i = 0; i < 4; i++) {
      JsonObject wm = wmArray[i];
      settings.wateringModes[i].enabled = wm["enabled"] | false;
      settings.wateringModes[i].startHour = wm["startHour"] | 8;
      settings.wateringModes[i].endHour = wm["endHour"] | 18;
      settings.wateringModes[i].duration = wm["duration"] | (5 * 60 * 1000UL);
    }
  }

  // ПРИМЕЧАНИЕ: В полной версии необходимо добавить все остальные поля

  return true;
}

// Загрузка настроек
bool StorageManager::loadSettings(Settings& settings) {
  LOG("StorageManager: Попытка загрузки настроек из %s\n", SETTINGS_FILE);

  File f = LittleFS.open(SETTINGS_FILE, "r");
  if (!f) {
    LOG("StorageManager: Не удалось открыть файл настроек. Применяю defaults.\n");
    applyDefaultSettings(settings);
    return false;
  }

  size_t size = f.size();
  if (size == 0) {
    LOG("StorageManager: Файл настроек пустой. Применяю defaults.\n");
    f.close();
    applyDefaultSettings(settings);
    return false;
  }

  DynamicJsonDocument doc(size + 512);
  DeserializationError error = deserializeJson(doc, f);
  f.close();

  if (error) {
    LOG("StorageManager: Ошибка парсинга JSON: %s. Применяю defaults.\n", error.c_str());
    applyDefaultSettings(settings);
    return false;
  }

  deserializeSettings(settings, doc);
  LOG("StorageManager: Настройки загружены успешно\n");
  return true;
}

// Сохранение настроек
bool StorageManager::saveSettings(const Settings& settings, const char* caller) {
  // Проверка cooldown
  if (millis() - _lastSaveTime < SAVE_COOLDOWN) {
    LOG("StorageManager: Сохранение пропущено (cooldown). Вызвано: %s\n", caller);
    return false;
  }

  _lastSaveTime = millis();

  LOG("StorageManager: Сохранение настроек. Вызвано: %s\n", caller);

  File file = LittleFS.open(SETTINGS_FILE, "w");
  if (!file) {
    LOG("StorageManager: Ошибка открытия файла для записи\n");
    return false;
  }

  DynamicJsonDocument doc(5120);
  serializeSettings(settings, doc);
  serializeJson(doc, file);
  file.close();

  LOG("StorageManager: Настройки сохранены успешно\n");
  return true;
}

// Загрузка истории
bool StorageManager::loadHistory(HistoryEntry* history, int& count, int maxSize) {
  LOG("StorageManager: Попытка загрузки истории из %s\n", HISTORY_FILE);

  File f = LittleFS.open(HISTORY_FILE, "r");
  if (!f) {
    LOG("StorageManager: Файл истории не найден\n");
    count = 0;
    return false;
  }

  size_t size = f.size();
  if (size == 0) {
    LOG("StorageManager: Файл истории пустой\n");
    f.close();
    count = 0;
    return false;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, f);
  f.close();

  if (error) {
    LOG("StorageManager: Ошибка парсинга истории: %s\n", error.c_str());
    count = 0;
    return false;
  }

  JsonArray arr = doc["history"];
  count = min((int)arr.size(), maxSize);

  for (int i = 0; i < count; i++) {
    JsonObject entry = arr[i];
    history[i].timestamp = entry["timestamp"] | 0;
    history[i].avgTemp = entry["avgTemp"] | 0.0f;
    history[i].avgHum = entry["avgHum"] | 0.0f;
    history[i].level = entry["level"] | 0.0f;
    history[i].windSpeed = entry["windSpeed"] | 0.0f;
    history[i].outdoorTemp = entry["outdoorTemp"] | 0.0f;
    history[i].outdoorHum = entry["outdoorHum"] | 0.0f;
    history[i].windowPosition = entry["windowPosition"] | 0.0f;
    history[i].fanState = entry["fanState"] | false;
    history[i].heatingState = entry["heatingState"] | false;
    history[i].wateringState = entry["wateringState"] | false;
    history[i].solutionHeatingState = entry["solutionHeatingState"] | false;
    history[i].hydroState = entry["hydroState"] | false;
    history[i].fillState = entry["fillState"] | false;
    history[i].fogState = entry["fogState"] | false;
  }

  LOG("StorageManager: История загружена (%d записей)\n", count);
  return true;
}

// Сохранение истории
bool StorageManager::saveHistory(const HistoryEntry* history, int count) {
  LOG("StorageManager: Сохранение истории (%d записей)\n", count);

  File file = LittleFS.open(HISTORY_FILE, "w");
  if (!file) {
    LOG("StorageManager: Ошибка открытия файла истории для записи\n");
    return false;
  }

  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("history");

  for (int i = 0; i < count; i++) {
    JsonObject entry = arr.createNestedObject();
    entry["timestamp"] = history[i].timestamp;
    entry["avgTemp"] = history[i].avgTemp;
    entry["avgHum"] = history[i].avgHum;
    entry["level"] = history[i].level;
    entry["windSpeed"] = history[i].windSpeed;
    entry["outdoorTemp"] = history[i].outdoorTemp;
    entry["outdoorHum"] = history[i].outdoorHum;
    entry["windowPosition"] = history[i].windowPosition;
    entry["fanState"] = history[i].fanState;
    entry["heatingState"] = history[i].heatingState;
    entry["wateringState"] = history[i].wateringState;
    entry["solutionHeatingState"] = history[i].solutionHeatingState;
    entry["hydroState"] = history[i].hydroState;
    entry["fillState"] = history[i].fillState;
    entry["fogState"] = history[i].fogState;
  }

  serializeJson(doc, file);
  file.close();

  LOG("StorageManager: История сохранена успешно\n");
  return true;
}
