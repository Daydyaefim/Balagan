// ===========================
// МОДУЛЬ УПРАВЛЕНИЯ MQTT - РЕАЛИЗАЦИЯ
// ===========================

#include "MqttManager.h"

// Инициализация статического указателя на экземпляр
MqttManager* MqttManager::_instance = nullptr;

// Конструктор
MqttManager::MqttManager(WiFiClient& wifiClient, RTC_DS3231& rtc)
  : _client(wifiClient), _rtc(rtc),
    _settingsUpdateCallback(nullptr), _commandCallback(nullptr) {
  _instance = this;
}

// Инициализация
void MqttManager::begin(const char* clientId) {
  _clientId = String(clientId);
  _client.setServer(MQTT_BROKER, 1883);
  _client.setCallback(mqttCallback);
  _client.setSocketTimeout(MQTT_CONNECT_TIMEOUT / 1000);
  LOG("MqttManager: Инициализация завершена\n");
}

// Подключение к MQTT брокеру
bool MqttManager::connect() {
  if (_client.connected()) {
    return true;
  }

  LOG("MqttManager: Попытка подключения к MQTT брокеру %s\n", MQTT_BROKER);

  if (_client.connect(_clientId.c_str())) {
    LOG("MqttManager: Подключение к MQTT брокеру успешно\n");

    // Подписка на топики
    subscribe();

    // Публикация статуса
    publishStatus("online");

    return true;
  }

  LOG("MqttManager: Не удалось подключиться к MQTT брокеру\n");
  return false;
}

// Проверка подключения
bool MqttManager::isConnected() const {
  return _client.connected();
}

// Основной цикл
void MqttManager::loop() {
  if (!_client.connected()) {
    // Попытка переподключения
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();

    if (now - lastReconnectAttempt > MQTT_RETRY_INTERVAL_DISCONNECTED) {
      lastReconnectAttempt = now;
      connect();
    }
  } else {
    _client.loop();
  }
}

// Подписка на топики
void MqttManager::subscribe() {
  _client.subscribe("greenhouse/set_settings");
  _client.subscribe("greenhouse/cmd/#");
  LOG("MqttManager: Подписка на топики выполнена\n");
}

// Статическая функция callback
void MqttManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (_instance != nullptr) {
    _instance->handleMessage(topic, payload, length);
  }
}

// Обработка сообщений
void MqttManager::handleMessage(char* topic, byte* payload, unsigned int length) {
  // Выделяем память для сообщения
  char* msg_buffer = (char*)malloc(length + 1);
  if (msg_buffer == nullptr) {
    LOG("ERROR: Недостаточно памяти для обработки MQTT сообщения\n");
    return;
  }

  memcpy(msg_buffer, payload, length);
  msg_buffer[length] = '\0';
  String msg = String(msg_buffer);
  free(msg_buffer);

  LOG("MqttManager: Получено сообщение [%s]: %s\n", topic, msg.c_str());

  // Обработка различных топиков
  if (strcmp(topic, "greenhouse/cmd/set_settings") == 0 ||
      strcmp(topic, "greenhouse/set_settings") == 0) {
    handleSetSettings(msg);
  } else if (strncmp(topic, "greenhouse/cmd/", 15) == 0) {
    handleCommand(topic + 15, msg);
  }
}

// Обработка команды установки настроек
void MqttManager::handleSetSettings(const String& json) {
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    LOG("MqttManager: Ошибка парсинга JSON настроек: %s\n", error.c_str());
    return;
  }

  // Здесь должна быть логика обновления настроек
  // В полной реализации нужно обработать все поля из документа
  // и вызвать callback для сохранения настроек

  LOG("MqttManager: Настройки получены из MQTT (требуется полная реализация обработки)\n");
}

// Обработка команд
void MqttManager::handleCommand(const char* topic, const String& value) {
  LOG("MqttManager: Команда [%s] = %s\n", topic, value.c_str());

  if (_commandCallback != nullptr) {
    _commandCallback(topic, value.c_str());
  }
}

// Публикация всех датчиков
void MqttManager::publishAllSensors(const SensorData& avgData, float sol, float level, float wind,
                                     const SensorData& outdoor, const MatSensorData& mat, float pyrano,
                                     int windowPos, const Settings& settings) {
  if (!_client.connected()) return;

  DynamicJsonDocument doc(4096);
  DateTime now = _rtc.now();

  doc["timestamp"] = now.unixtime() - 10800; // UTC-3
  doc["temperature"] = avgData.temperature;
  doc["humidity"] = avgData.humidity;
  doc["water_level"] = level;
  doc["wind_speed"] = wind;
  doc["pyrano"] = isnan(pyrano) ? 0 : pyrano;
  doc["solution_temperature"] = sol;
  doc["outdoor_temperature"] = outdoor.temperature;
  doc["outdoor_humidity"] = outdoor.humidity;
  doc["mat_temperature"] = mat.temperature;
  doc["mat_hum"] = mat.humidity;
  doc["mat_ec"] = mat.ec;
  doc["mat_ph"] = mat.ph;
  doc["window_position"] = windowPos;
  doc["fan_state"] = settings.fanState;
  doc["heat_state"] = settings.heatState;
  doc["pump_state"] = settings.pumpState;
  doc["fog_state"] = settings.fogState;
  doc["sol_heat_state"] = settings.solHeatState;
  doc["hydro_mix"] = settings.hydroMix;
  doc["fill_state"] = settings.fillState;
  doc["watering_mode"] = settings.wateringMode;
  doc["rad_sum"] = settings.radSum;
  doc["cycle_count"] = settings.cycleCount;
  doc["last_watering_start_unix"] = settings.lastWateringStartUnix;
  doc["force_watering_active"] = settings.forceWateringActive;
  doc["force_watering_override"] = settings.forceWateringOverride;
  doc["force_watering_end_time_unix"] = settings.forceWateringEndTimeUnix;

  String json;
  serializeJson(doc, json);
  _client.publish("greenhouse/esp32/all_sensors", json.c_str(), true);
}

// Публикация настроек
void MqttManager::publishSettings(const Settings& settings) {
  if (!_client.connected()) return;

  DynamicJsonDocument doc(5120);

  // Температура и влажность
  doc["minTemp"] = settings.minTemp;
  doc["maxTemp"] = settings.maxTemp;
  doc["minHum"] = settings.minHum;
  doc["maxHum"] = settings.maxHum;

  // Окна
  doc["openTime"] = settings.openTime;
  doc["closeTime"] = settings.closeTime;
  doc["manualWindow"] = settings.manualWindow;

  // Отопление
  doc["heatOn"] = settings.heatOn;
  doc["heatOff"] = settings.heatOff;
  doc["manualHeat"] = settings.manualHeat;
  doc["heatState"] = settings.heatState;

  // Вентилятор
  doc["fanInterval"] = settings.fanInterval;
  doc["fanDuration"] = settings.fanDuration;
  doc["manualFan"] = settings.manualFan;
  doc["fanState"] = settings.fanState;

  // Туманообразование
  doc["fogMode"] = settings.fogMode;
  doc["fogMorningStart"] = settings.fogMorningStart;
  doc["fogMorningEnd"] = settings.fogMorningEnd;
  doc["fogDayStart"] = settings.fogDayStart;
  doc["fogDayEnd"] = settings.fogDayEnd;
  doc["fogMorningDuration"] = settings.fogMorningDuration;
  doc["fogMorningInterval"] = settings.fogMorningInterval;
  doc["fogDayDuration"] = settings.fogDayDuration;
  doc["fogDayInterval"] = settings.fogDayInterval;
  doc["forceFogOn"] = settings.forceFogOn;
  doc["fogState"] = settings.fogState;

  // Раствор
  doc["solOn"] = settings.solOn;
  doc["solOff"] = settings.solOff;
  doc["manualSol"] = settings.manualSol;
  doc["solHeatState"] = settings.solHeatState;

  // Ветер
  doc["wind1"] = settings.wind1;
  doc["wind2"] = settings.wind2;
  doc["windLockMinutes"] = settings.windLockMinutes;

  // День/ночь
  doc["nightStart"] = settings.nightStart;
  doc["dayStart"] = settings.dayStart;
  doc["nightOffset"] = settings.nightOffset;

  // Уровень
  doc["levelMin"] = settings.levelMin;
  doc["levelMax"] = settings.levelMax;

  // Полив
  doc["manualPump"] = settings.manualPump;
  doc["pumpState"] = settings.pumpState;
  doc["wateringMode"] = settings.wateringMode;

  String json;
  serializeJson(doc, json);
  _client.publish("greenhouse/esp32/settings", json.c_str(), true);

  LOG("MqttManager: Настройки опубликованы\n");
}

// Публикация истории
void MqttManager::publishHistory(const HistoryEntry* history, int size) {
  if (!_client.connected()) return;

  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("history");

  for (int i = 0; i < size; i++) {
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

  String json;
  serializeJson(doc, json);
  _client.publish("greenhouse/esp32/history", json.c_str(), true);

  LOG("MqttManager: История опубликована\n");
}

// Публикация позиции окна
void MqttManager::publishWindowPosition(int position) {
  if (!_client.connected()) return;
  _client.publish("greenhouse/esp32/window", String(position).c_str(), true);
}

// Публикация статуса
void MqttManager::publishStatus(const char* status) {
  if (!_client.connected()) return;
  _client.publish("greenhouse/esp32/status", status, true);
  LOG("MqttManager: Статус опубликован: %s\n", status);
}

// Установка callback'ов
void MqttManager::setSettingsUpdateCallback(SettingsUpdateCallback callback) {
  _settingsUpdateCallback = callback;
}

void MqttManager::setCommandCallback(CommandCallback callback) {
  _commandCallback = callback;
}
