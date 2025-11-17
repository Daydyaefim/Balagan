// ===========================
// МОДУЛЬ УПРАВЛЕНИЯ ИСПОЛНИТЕЛЬНЫМИ УСТРОЙСТВАМИ
// ===========================

#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>
#include "Config.h"

// Класс для управления исполнительными устройствами (реле)
class ActuatorManager {
public:
  // Конструктор
  ActuatorManager();

  // Инициализация всех реле
  void begin();

  // Калибровка окна (закрытие до упора при старте)
  void calibrateWindow();

  // Управление окнами
  void openWindow();
  void closeWindow();
  void stopWindow();
  bool isWindowMoving() const { return _windowMoving; }
  int getWindowPosition() const { return _windowPos; }
  void setWindowPosition(int pos) { _windowPos = pos; }
  void updateWindowPosition(uint32_t currentTime);
  void startWindowMovement(String direction, uint32_t duration, uint32_t currentTime);

  // Управление отоплением
  void setHeating(bool state);
  bool getHeatingState() const { return _heatState; }

  // Управление вентилятором
  void setFan(bool state);
  bool getFanState() const { return _fanState; }

  // Управление туманообразованием
  void setFogValve(bool state);
  void setFogPump(bool state);
  bool getFogValveState() const { return _fogValveState; }
  bool getFogPumpState() const { return _fogPumpState; }

  // Управление поливом
  void setPump(bool state);
  void setFillValve(bool state);
  void setThreeWayValve(bool state);
  bool getPumpState() const { return _pumpState; }
  bool getFillState() const { return _fillState; }
  bool getThreeWayState() const { return _threeWayState; }

  // Управление нагревом раствора
  void setSolutionHeating(bool state);
  bool getSolutionHeatingState() const { return _solHeatState; }

  // Аварийное отключение всех реле
  void emergencyStopAll();

private:
  // Состояния реле
  bool _heatState;
  bool _fanState;
  bool _fogValveState;
  bool _fogPumpState;
  bool _pumpState;
  bool _fillState;
  bool _threeWayState;
  bool _solHeatState;

  // Состояние окна
  int _windowPos;           // Текущая позиция окна (0-100%)
  bool _windowMoving;       // Флаг движения окна
  String _windowDirection;  // Направление движения ("up" или "down")
  uint32_t _windowStartTime; // Время начала движения
  int _targetWindowPos;     // Целевая позиция
  uint32_t _moveDuration;   // Длительность движения

  // Константы для калибровки окна
  static const uint32_t WINDOW_FULL_CLOSE_TIME = 156000; // 156 секунд
  static const uint32_t WINDOW_EXTRA_CLOSE_TIME = 5000;  // 5 секунд дополнительно
};

#endif // ACTUATORS_H
