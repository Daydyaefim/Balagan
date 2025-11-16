// ===========================
// МОДУЛЬ УПРАВЛЕНИЯ ИСПОЛНИТЕЛЬНЫМИ УСТРОЙСТВАМИ - РЕАЛИЗАЦИЯ
// ===========================

#include "Actuators.h"

// Конструктор
ActuatorManager::ActuatorManager()
  : _heatState(false), _fanState(false), _fogValveState(false), _fogPumpState(false),
    _pumpState(false), _fillState(false), _threeWayState(false), _solHeatState(false),
    _windowPos(0), _windowMoving(false), _windowDirection(""), _windowStartTime(0),
    _targetWindowPos(0), _moveDuration(0) {
}

// Инициализация всех реле
void ActuatorManager::begin() {
  // Настройка пинов реле как выходы
  pinMode(REL_WINDOW_UP, OUTPUT);
  pinMode(REL_WINDOW_DN, OUTPUT);
  pinMode(REL_HEAT, OUTPUT);
  pinMode(REL_FAN, OUTPUT);
  pinMode(REL_FOG_VALVE, OUTPUT);
  pinMode(REL_FOG_PUMP, OUTPUT);
  pinMode(REL_PUMP, OUTPUT);
  pinMode(REL_FILL, OUTPUT);
  pinMode(REL_3WAY, OUTPUT);
  pinMode(REL_SOL, OUTPUT);

  // Установка всех реле в выключенное состояние
  digitalWrite(REL_WINDOW_UP, LOW);
  digitalWrite(REL_WINDOW_DN, LOW);
  digitalWrite(REL_HEAT, LOW);
  digitalWrite(REL_FAN, LOW);
  digitalWrite(REL_FOG_VALVE, LOW);
  digitalWrite(REL_FOG_PUMP, LOW);
  digitalWrite(REL_PUMP, LOW);
  digitalWrite(REL_FILL, LOW);
  digitalWrite(REL_3WAY, LOW);
  digitalWrite(REL_SOL, LOW);

  LOG("ActuatorManager: Все реле инициализированы и выключены\n");
}

// Калибровка окна (закрытие до упора при старте)
void ActuatorManager::calibrateWindow() {
  LOG("ActuatorManager: Начало калибровки окна (полное закрытие)\n");

  // Включаем реле закрытия окна
  digitalWrite(REL_WINDOW_DN, HIGH);

  // Ждем полного закрытия + дополнительное время для гарантии
  delay(WINDOW_FULL_CLOSE_TIME + WINDOW_EXTRA_CLOSE_TIME);

  // Выключаем реле
  digitalWrite(REL_WINDOW_DN, LOW);

  // Устанавливаем позицию окна в 0%
  _windowPos = 0;

  LOG("ActuatorManager: Калибровка окна завершена, позиция установлена в 0%%\n");
}

// Управление окнами
void ActuatorManager::openWindow() {
  digitalWrite(REL_WINDOW_UP, HIGH);
  digitalWrite(REL_WINDOW_DN, LOW);
  LOG("ActuatorManager: Окно открывается\n");
}

void ActuatorManager::closeWindow() {
  digitalWrite(REL_WINDOW_UP, LOW);
  digitalWrite(REL_WINDOW_DN, HIGH);
  LOG("ActuatorManager: Окно закрывается\n");
}

void ActuatorManager::stopWindow() {
  digitalWrite(REL_WINDOW_UP, LOW);
  digitalWrite(REL_WINDOW_DN, LOW);
  _windowMoving = false;
  _moveDuration = 0;
  LOG("ActuatorManager: Движение окна остановлено\n");
}

void ActuatorManager::startWindowMovement(String direction, uint32_t duration, uint32_t currentTime) {
  _windowMoving = true;
  _windowDirection = direction;
  _windowStartTime = currentTime;
  _moveDuration = duration;

  if (direction == "up") {
    openWindow();
    LOG("ActuatorManager: Начало движения окна ВВЕРХ на %u мс\n", duration);
  } else if (direction == "down") {
    closeWindow();
    LOG("ActuatorManager: Начало движения окна ВНИЗ на %u мс\n", duration);
  }
}

void ActuatorManager::updateWindowPosition(uint32_t currentTime) {
  if (!_windowMoving) return;

  uint32_t elapsed = currentTime - _windowStartTime;

  // Если время движения истекло, останавливаем окно
  if (elapsed >= _moveDuration) {
    // Вычисляем процентное изменение позиции
    // Примечание: для точного расчета нужно знать полное время открытия/закрытия
    // Здесь используется упрощенная логика
    float percentageChange = ((float)_moveDuration / 1560.0f); // 1560 мс = 1% движения (156000мс / 100%)

    if (_windowDirection == "up") {
      _windowPos = constrain(_windowPos + (int)percentageChange, 0, 100);
    } else if (_windowDirection == "down") {
      _windowPos = constrain(_windowPos - (int)percentageChange, 0, 100);
    }

    stopWindow();
    LOG("ActuatorManager: Окно остановлено. Прошло: %u мс, Новая позиция: %d%%\n", elapsed, _windowPos);
  }
}

// Управление отоплением
void ActuatorManager::setHeating(bool state) {
  _heatState = state;
  digitalWrite(REL_HEAT, state ? HIGH : LOW);
  LOG("ActuatorManager: Отопление %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

// Управление вентилятором
void ActuatorManager::setFan(bool state) {
  _fanState = state;
  digitalWrite(REL_FAN, state ? HIGH : LOW);
  LOG("ActuatorManager: Вентилятор %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

// Управление туманообразованием
void ActuatorManager::setFogValve(bool state) {
  _fogValveState = state;
  digitalWrite(REL_FOG_VALVE, state ? HIGH : LOW);
  LOG("ActuatorManager: Клапан тумана %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

void ActuatorManager::setFogPump(bool state) {
  _fogPumpState = state;
  digitalWrite(REL_FOG_PUMP, state ? HIGH : LOW);
  LOG("ActuatorManager: Насос тумана %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

// Управление поливом
void ActuatorManager::setPump(bool state) {
  _pumpState = state;
  digitalWrite(REL_PUMP, state ? HIGH : LOW);
  LOG("ActuatorManager: Насос полива %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

void ActuatorManager::setFillValve(bool state) {
  _fillState = state;
  digitalWrite(REL_FILL, state ? HIGH : LOW);
  LOG("ActuatorManager: Клапан наполнения %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

void ActuatorManager::setThreeWayValve(bool state) {
  _threeWayState = state;
  digitalWrite(REL_3WAY, state ? HIGH : LOW);
  LOG("ActuatorManager: 3-ходовой клапан %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

// Управление нагревом раствора
void ActuatorManager::setSolutionHeating(bool state) {
  _solHeatState = state;
  digitalWrite(REL_SOL, state ? HIGH : LOW);
  LOG("ActuatorManager: Нагрев раствора %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

// Аварийное отключение всех реле
void ActuatorManager::emergencyStopAll() {
  LOG("ActuatorManager: АВАРИЙНОЕ ОТКЛЮЧЕНИЕ ВСЕХ РЕЛЕ!\n");

  digitalWrite(REL_WINDOW_UP, LOW);
  digitalWrite(REL_WINDOW_DN, LOW);
  digitalWrite(REL_HEAT, LOW);
  digitalWrite(REL_FAN, LOW);
  digitalWrite(REL_FOG_VALVE, LOW);
  digitalWrite(REL_FOG_PUMP, LOW);
  digitalWrite(REL_PUMP, LOW);
  digitalWrite(REL_FILL, LOW);
  digitalWrite(REL_3WAY, LOW);
  digitalWrite(REL_SOL, LOW);

  _heatState = false;
  _fanState = false;
  _fogValveState = false;
  _fogPumpState = false;
  _pumpState = false;
  _fillState = false;
  _threeWayState = false;
  _solHeatState = false;
  _windowMoving = false;
}
