// ===========================
// МОДУЛЬ РАБОТЫ С ДАТЧИКАМИ - РЕАЛИЗАЦИЯ
// ===========================

#include "Sensors.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Глобальный указатель на менеджер датчиков для использования в задаче FreeRTOS
static SensorManager* g_sensorManager = nullptr;

// Конструктор
SensorManager::SensorManager(ModbusMaster& modbus, HardwareSerial& serial)
  : _mb(modbus), _serial(serial),
    _qAvg(nullptr), _qSol(nullptr), _qLevel(nullptr),
    _qWind(nullptr), _qPyrano(nullptr), _qMat(nullptr), _qOutdoor(nullptr),
    _levelMin(0), _levelMax(1000) {
}

// Инициализация
void SensorManager::begin() {
  _serial.begin(MODBUS_BAUD, SERIAL_8N1, MODBUS_RX, MODBUS_TX);
  LOG("SensorManager: Modbus инициализирован на скорости %d\n", MODBUS_BAUD);
}

// Настройка очередей
void SensorManager::setQueues(QueueHandle_t qAvg, QueueHandle_t qSol, QueueHandle_t qLevel,
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

// Чтение 16-битного значения по Modbus
uint16_t SensorManager::readU16(uint8_t slaveId, uint16_t reg) {
  _mb.begin(slaveId, _serial);
  uint8_t result = _mb.readHoldingRegisters(reg, 1);

  uint32_t t0 = millis();
  const uint32_t timeout = 1000;

  while (_mb.getResponseBuffer(0) == 0 && result == _mb.ku8MBResponseTimedOut && millis() - t0 < timeout) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  if (result == _mb.ku8MBSuccess) {
    uint16_t value = _mb.getResponseBuffer(0);
    LOG("Modbus read success for ID %d, reg %d: %u\n", slaveId, reg, value);
    return value;
  }

  LOG("ERROR: Modbus read failed for ID %d, reg %d, code: 0x%02X\n", slaveId, reg, result);
  return 65535;
}

// Чтение float значения по Modbus
float SensorManager::readFloat(uint8_t slaveId, uint16_t reg) {
  _mb.begin(slaveId, _serial);
  uint8_t result = _mb.readHoldingRegisters(reg, 2);

  uint32_t t0 = millis();
  const uint32_t timeout = 1000;

  while (_mb.getResponseBuffer(0) == 0 && result == _mb.ku8MBResponseTimedOut && millis() - t0 < timeout) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  if (result == _mb.ku8MBSuccess) {
    uint32_t raw = ((uint32_t)_mb.getResponseBuffer(1) << 16) | _mb.getResponseBuffer(0);
    float value = *(float*)&raw;

    if (value < -40 || value > 1000) {
      LOG("ERROR: Invalid value for ID %d, reg %d: %.2f\n", slaveId, reg, value);
      return NAN;
    }

    LOG("Modbus read success for ID %d, reg %d: %.2f\n", slaveId, reg, value);
    return value;
  }

  LOG("ERROR: Modbus read failed for ID %d, reg %d, code: 0x%02X\n", slaveId, reg, result);
  return NAN;
}

// Функция для map с float
float SensorManager::mapValue(float x, float inMin, float inMax, float outMin, float outMax) {
  if (inMax == inMin) return outMin;
  float result = (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
  return constrain(result, min(outMin, outMax), max(outMin, outMax));
}

// Чтение датчиков температуры и влажности
void SensorManager::readTemperatureHumidity() {
  LOG("--- Начало чтения датчиков T/H (ID %d & %d) ---\n", SLAVE_TEMP1, SLAVE_TEMP2);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Датчик 1
  uint16_t raw_h1 = readU16(SLAVE_TEMP1, 0);
  float h1 = (raw_h1 == 65535 || raw_h1 == 0 || raw_h1 > 1100) ? NAN : raw_h1 / 10.0f;
  LOG("H1 raw=%u → h1=%.1f%% (valid: %s)\n", raw_h1, h1, isnan(h1) ? "false" : "true");

  uint16_t raw_t1 = readU16(SLAVE_TEMP1, 1);
  float t1 = (raw_t1 == 65535 || raw_t1 == 0 || raw_t1 < -100 || raw_t1 > 600) ? NAN : raw_t1 / 10.0f;
  LOG("T1 raw=%u → t1=%.1f°C (valid: %s)\n", raw_t1, t1, isnan(t1) ? "false" : "true");

  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Датчик 2
  uint16_t raw_h2 = readU16(SLAVE_TEMP2, 0);
  float h2 = (raw_h2 == 65535 || raw_h2 == 0 || raw_h2 > 1100) ? NAN : raw_h2 / 10.0f;
  LOG("H2 raw=%u → h2=%.1f%% (valid: %s)\n", raw_h2, h2, isnan(h2) ? "false" : "true");

  uint16_t raw_t2 = readU16(SLAVE_TEMP2, 1);
  float t2 = (raw_t2 == 65535 || raw_t2 == 0 || raw_t2 < -100 || raw_t2 > 600) ? NAN : raw_t2 / 10.0f;
  LOG("T2 raw=%u → t2=%.1f°C (valid: %s)\n", raw_t2, t2, isnan(t2) ? "false" : "true");

  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Усреднение
  float avgHum = (isnan(h1) || isnan(h2)) ? (isnan(h1) ? (isnan(h2) ? NAN : h2) : h1) : (h1 + h2) / 2.0f;
  float avgTemp = (isnan(t1) || isnan(t2)) ? (isnan(t1) ? (isnan(t2) ? NAN : t2) : t1) : (t1 + t2) / 2.0f;

  LOG("Усреднённые: H=%.1f%% (h1=%.1f, h2=%.1f), T=%.1f°C (t1=%.1f, t2=%.1f)\n", avgHum, h1, h2, avgTemp, t1, t2);

  // Отправка в очередь
  if (_qAvg != nullptr) {
    SensorData avgData = { avgTemp, avgHum };
    xQueueOverwrite(_qAvg, &avgData);
  }
}

// Чтение температуры раствора
void SensorManager::readSolutionTemperature() {
  vTaskDelay(100 / portTICK_PERIOD_MS);
  uint16_t raw_sol = readU16(SLAVE_SOL, 0);
  float sol = raw_sol / 10.0f;
  LOG("Датчик раствора (ID %d): T=%.1f°C (сырое: %u)\n", SLAVE_SOL, sol, raw_sol);

  if (_qSol != nullptr) {
    xQueueOverwrite(_qSol, &sol);
  }
}

// Чтение датчика уровня
void SensorManager::readLevelSensor() {
  vTaskDelay(100 / portTICK_PERIOD_MS);
  uint16_t rawLevel = readU16(SLAVE_LEVEL, 4);
  float level = mapValue(rawLevel, _levelMin, _levelMax, 0, 100);
  level = constrain(level, 0, 100);
  LOG("Датчик уровня (ID %d): Level=%.1f%% (сырое: %u, Min: %.1f, Max: %.1f)\n",
      SLAVE_LEVEL, level, rawLevel, _levelMin, _levelMax);

  if (_qLevel != nullptr) {
    xQueueOverwrite(_qLevel, &level);
  }
}

// Чтение датчика ветра
void SensorManager::readWindSensor() {
  vTaskDelay(100 / portTICK_PERIOD_MS);
  uint16_t raw_wind = readU16(SLAVE_WIND, 0);
  float wind = raw_wind / 10.0f;
  LOG("Датчик ветра (ID %d): Wind=%.1f m/s (сырое: %u)\n", SLAVE_WIND, wind, raw_wind);

  if (_qWind != nullptr) {
    xQueueOverwrite(_qWind, &wind);
  }
}

// Чтение пиранометра
void SensorManager::readPyranometer() {
  vTaskDelay(100 / portTICK_PERIOD_MS);
  uint16_t raw_pyrano = readU16(SLAVE_PYRANO, 0);
  float pyrano = (float)raw_pyrano;
  LOG("Пиранометр (ID %d): Pyrano=%.2f W/m2\n", SLAVE_PYRANO, pyrano);

  if (_qPyrano != nullptr) {
    xQueueOverwrite(_qPyrano, &pyrano);
  }
}

// Чтение датчика субстрата (мат-сенсор)
void SensorManager::readMatSensor() {
  vTaskDelay(100 / portTICK_PERIOD_MS);

  uint16_t raw_hum = readU16(SLAVE_MAT_SENSOR, 0);
  uint16_t raw_temp = readU16(SLAVE_MAT_SENSOR, 1);
  uint16_t raw_ec = readU16(SLAVE_MAT_SENSOR, 2);
  uint16_t raw_ph = readU16(SLAVE_MAT_SENSOR, 3);

  MatSensorData matData;
  matData.humidity = raw_hum / 10.0f;
  matData.temperature = raw_temp / 10.0f;
  matData.ec = raw_ec / 100.0f;
  matData.ph = raw_ph / 10.0f;

  LOG("Датчик мата (ID %d): H=%.1f%%, T=%.1f°C, EC=%u µS/cm, pH=%.2f\n",
      SLAVE_MAT_SENSOR, matData.humidity, matData.temperature, (uint16_t)matData.ec, matData.ph);

  if (_qMat != nullptr) {
    xQueueOverwrite(_qMat, &matData);
  }
}

// Чтение наружного датчика
void SensorManager::readOutdoorSensor() {
  LOG("- Начало чтения уличного датчика T/H (ID %d) -\n", SLAVE_OUTDOOR);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  uint16_t raw_outdoor_h = readU16(SLAVE_OUTDOOR, 0);
  float outdoorHum = raw_outdoor_h / 10.0f;

  uint16_t raw_outdoor_t = readU16(SLAVE_OUTDOOR, 1);
  float outdoorTemp = raw_outdoor_t / 10.0f;

  LOG("Датчик улицы (ID %d): H=%.1f%%, T=%.1f°C (сырое H: %u, T: %u)\n",
      SLAVE_OUTDOOR, outdoorHum, outdoorTemp, raw_outdoor_h, raw_outdoor_t);

  if (_qOutdoor != nullptr) {
    SensorData outdoorData = { outdoorTemp, outdoorHum };
    xQueueOverwrite(_qOutdoor, &outdoorData);
  }
}

// Чтение всех датчиков
void SensorManager::readAllSensors() {
  readTemperatureHumidity();
  LOG("--- Начало чтения других датчиков (uint16_t) ---\n");
  readSolutionTemperature();
  readLevelSensor();
  readWindSensor();
  LOG("--- Начало чтения НОВЫХ датчиков (float) ---\n");
  readPyranometer();
  readMatSensor();
  readOutdoorSensor();
}

// Получение данных из очередей
bool SensorManager::getAverageSensorData(SensorData& data) {
  if (_qAvg == nullptr) return false;
  return xQueuePeek(_qAvg, &data, 0) == pdTRUE;
}

bool SensorManager::getSolutionTemperature(float& temp) {
  if (_qSol == nullptr) return false;
  return xQueuePeek(_qSol, &temp, 0) == pdTRUE;
}

bool SensorManager::getLevel(float& level) {
  if (_qLevel == nullptr) return false;
  return xQueuePeek(_qLevel, &level, 0) == pdTRUE;
}

bool SensorManager::getWindSpeed(float& wind) {
  if (_qWind == nullptr) return false;
  return xQueuePeek(_qWind, &wind, 0) == pdTRUE;
}

bool SensorManager::getPyranometer(float& pyrano) {
  if (_qPyrano == nullptr) return false;
  return xQueuePeek(_qPyrano, &pyrano, 0) == pdTRUE;
}

bool SensorManager::getMatSensorData(MatSensorData& data) {
  if (_qMat == nullptr) return false;
  return xQueuePeek(_qMat, &data, 0) == pdTRUE;
}

bool SensorManager::getOutdoorSensorData(SensorData& data) {
  if (_qOutdoor == nullptr) return false;
  return xQueuePeek(_qOutdoor, &data, 0) == pdTRUE;
}

// Задача FreeRTOS для чтения датчиков
void modbusTask(void* parameter) {
  SensorManager* sensorMgr = (SensorManager*)parameter;

  for (;;) {
    sensorMgr->readAllSensors();
    vTaskDelay(3000 / portTICK_PERIOD_MS); // Пауза 3 секунды между циклами чтения
  }
}
