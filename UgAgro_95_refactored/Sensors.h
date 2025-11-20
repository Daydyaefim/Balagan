// ===========================
// МОДУЛЬ РАБОТЫ С ДАТЧИКАМИ
// ===========================

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <ModbusMaster.h>
#include "Config.h"

// Класс для работы с датчиками через Modbus
class SensorManager {
public:
  // Конструктор
  SensorManager(ModbusMaster& modbus, HardwareSerial& serial);

  // Инициализация
  void begin();

  // Чтение данных с датчиков (должно вызываться в отдельной задаче FreeRTOS)
  void readAllSensors();

  // Получение данных из очередей
  bool getAverageSensorData(SensorData& data);
  bool getSolutionTemperature(float& temp);
  bool getLevel(float& level);
  bool getWindSpeed(float& wind);
  bool getPyranometer(float& pyrano);
  bool getMatSensorData(MatSensorData& data);
  bool getOutdoorSensorData(SensorData& data);

  // Настройка очередей FreeRTOS для передачи данных
  void setQueues(QueueHandle_t qAvg, QueueHandle_t qSol, QueueHandle_t qLevel,
                 QueueHandle_t qWind, QueueHandle_t qPyrano, QueueHandle_t qMat,
                 QueueHandle_t qOutdoor);

private:
  ModbusMaster& _mb;
  HardwareSerial& _serial;

  // Очереди для хранения данных датчиков
  QueueHandle_t _qAvg;
  QueueHandle_t _qSol;
  QueueHandle_t _qLevel;
  QueueHandle_t _qWind;
  QueueHandle_t _qPyrano;
  QueueHandle_t _qMat;
  QueueHandle_t _qOutdoor;

  // Калибровочные параметры уровня
  float _levelMin;
  float _levelMax;

  // Вспомогательные функции чтения Modbus
  uint16_t readU16(uint8_t slaveId, uint16_t reg);
  float readFloat(uint8_t slaveId, uint16_t reg);

  // Чтение конкретных датчиков
  void readTemperatureHumidity();
  void readSolutionTemperature();
  void readLevelSensor();
  void readWindSensor();
  void readPyranometer();
  void readMatSensor();
  void readOutdoorSensor();

  // Вспомогательные функции
  float mapValue(float x, float inMin, float inMax, float outMin, float outMax);
};

// Задача FreeRTOS для чтения датчиков
void modbusTask(void* parameter);

#endif // SENSORS_H
