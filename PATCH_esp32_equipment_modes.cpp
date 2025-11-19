// =====================================================
// PATCH: Add equipment modes to MQTT messages
// =====================================================
// Файл: UgAgro_95_refactored/MqttManager.cpp
//
// Добавьте следующие строки в функцию publishAllSensors
// ПОСЛЕ строки 194 (doc["hydro_mix"] = settings.hydroMix;)
// =====================================================

// Добавляем режимы работы оборудования
doc["fan_mode"] = settings.manualFan ? "manual" : "auto";
doc["heat_mode"] = settings.manualHeat ? "manual" : "auto";

// Режим тумана: 0=auto, 1=manual, 2=forced
const char* fogModeStr = "auto";
if (settings.fogMode == 1) fogModeStr = "manual";
else if (settings.fogMode == 2) fogModeStr = "forced";
doc["fog_mode"] = fogModeStr;

// Гидромикс всегда в auto режиме (нет ручного управления в текущей версии)
doc["hydro_mix_mode"] = "auto";

// ПРИМЕЧАНИЕ: watering_mode уже отправляется как INTEGER (строка 196)
// Не нужно отправлять pump_mode - это просто состояние насоса (state)

// =====================================================
// РЕЗУЛЬТАТ: ESP32 будет отправлять режимы в формате:
// {
//   "fan_state": true/false,
//   "fan_mode": "auto" | "manual",
//   "heat_state": true/false,
//   "heat_mode": "auto" | "manual",
//   "pump_state": true/false,           // Только состояние, без режима
//   "fog_state": true/false,
//   "fog_mode": "auto" | "manual" | "forced",
//   "hydro_mix": true/false,
//   "hydro_mix_mode": "auto",
//   "watering_mode": 0 | 1 | 2          // 0=auto, 1=manual, 2=forced
// }
// =====================================================
