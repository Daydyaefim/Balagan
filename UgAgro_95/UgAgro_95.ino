// ===========================
// УМНАЯ ТЕПЛИЦА – ПОЛНАЯ ПРОШИВКА
// 2025-11-06 (исправления: сброс таймера/radSum только после реального forced-полива; radSum не сбрасывается в morning; minInterval от последнего morning после последовательности; явная публикация MQTT после переходов)
// ===========================
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ModbusMaster.h>
#include <algorithm>
#include <Wire.h>
#include <RTClib.h>
#define DEBUG 0
#if DEBUG
#define LOG(x...) Serial.printf(x)
#else
#define LOG(...)
#endif
RTC_DS3231 rtc;
#define MODBUS_RX 16
#define MODBUS_TX 17
#define MODBUS_BAUD 19200
HardwareSerial RS485(1);
ModbusMaster mb;
#define SLAVE_TEMP1 1
#define SLAVE_TEMP2 2
#define SLAVE_WIND 3
#define SLAVE_PYRANO 5
#define SLAVE_MAT_SENSOR 7
#define SLAVE_SOL 12
#define SLAVE_LEVEL 14
#define SLAVE_OUTDOOR 15
#define REL_WINDOW_UP 33
#define REL_WINDOW_DN 32
#define REL_HEAT 25
#define REL_FAN 26
#define REL_FOG_VALVE 27
#define REL_FOG_PUMP 13
#define REL_PUMP 14
#define REL_FILL 18
#define REL_3WAY 12
#define REL_SOL 15
#define MQTT_CONNECT_TIMEOUT 3000
#define MQTT_RETRY_INTERVAL_DISCONNECTED 10000
#define MQTT_RETRY_INTERVAL_CONNECTED 60000
const char *WIFI_SSID = "2G_WIFI_05E0";
const char *WIFI_PASS = "12345678";
const char *MQTT_BROKER = "broker.emqx.io";
WiFiClient net;
PubSubClient client(net);
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 10800);
QueueHandle_t qAvg, qSol, qLevel, qWind, qOutdoor;
QueueHandle_t qPyrano;
QueueHandle_t qMat;
SemaphoreHandle_t mtx;
#define HISTORY_SIZE 20
struct HistoryEntry {
  uint32_t timestamp;
  float avgTemp;
  float avgHum;
  float level;
  float windSpeed;
  float outdoorTemp;
  float outdoorHum;
  float windowPosition;
  bool fanState;
  bool heatingState;
  bool wateringState;
  bool solutionHeatingState;
  bool hydroState;
  bool fillState;
  bool fogState;
};
HistoryEntry history[HISTORY_SIZE];
int historyIndex = 0;
extern TaskHandle_t networkTimeTaskHandle;
const unsigned long NETWORK_CHECK_INTERVAL = 30000;
const unsigned long TIME_UPDATE_INTERVAL = 30 * 60 * 1000;
extern volatile bool forceWifiReconnect;
extern volatile bool wifiConnectedOnce;
extern unsigned long lastNetworkCheckTime;
extern unsigned long lastTimeUpdateTime;
TaskHandle_t networkTimeTaskHandle = NULL;
volatile bool forceWifiReconnect = false;
volatile bool wifiConnectedOnce = false;
unsigned long lastNetworkCheckTime = 0;
unsigned long lastTimeUpdateTime = 0;
struct WateringMode {
  bool enabled;
  uint8_t startHour;
  uint8_t endHour;
  uint32_t duration;
};
struct Settings {
  float minTemp;
  float maxTemp;
  float minHum;
  float maxHum;
  uint32_t openTime;
  uint32_t closeTime;
  float heatOn;
  float heatOff;
  uint32_t fanInterval;
  uint32_t fanDuration;
  uint8_t fogMorningStart;
  uint8_t fogMorningEnd;
  uint8_t fogDayStart;
  uint8_t fogDayEnd;
  uint32_t fogMorningDuration;
  uint32_t fogMorningInterval;
  uint32_t fogDayDuration;
  uint32_t fogDayInterval;
  uint32_t fogDelay;
  float fogMinHum;
  float fogMaxHum;
  float fogMinTemp;
  float fogMaxTemp;
  uint32_t fogMinDuration;
  uint32_t fogMaxDuration;
  uint32_t fogMinInterval;
  uint32_t fogMaxInterval;
  float solOn;
  float solOff;
  float wind1;
  float wind2;
  uint32_t windLockMinutes;
  uint8_t nightStart;
  uint8_t dayStart;
  float nightOffset;
  bool manualWindow;
  bool manualHeat;
  bool manualPump;
  bool manualSol;
  int fogMode;
  bool manualFan;
  uint32_t hydroMixDuration;
  bool previousManualPump;
  bool manualPumpOverride;
  bool forceFogOn;
  uint32_t autoFogDayStart;
  uint32_t autoFogDayEnd;
  float levelMin;
  float levelMax;
  bool heatState;
  bool fanState;
  bool fogState;
  bool pumpState;
  bool solHeatState;
  bool hydroMix;
  uint32_t hydroStart;
  bool fillState;
  WateringMode wateringModes[4];
  bool forceWateringActive = false;
  bool forceWateringOverride = false;
  uint32_t forceWateringEndTime = 0;
  int wateringMode;
  uint32_t manualWateringInterval;
  uint32_t manualWateringDuration;
  float matMinHumidity;
  float matMaxEC;
  float radThreshold;
  uint32_t pumpTime;
  uint8_t wateringStartHour;
  uint8_t wateringStartMinute;
  uint8_t wateringEndHour;
  uint8_t wateringEndMinute;
  uint8_t maxWateringCycles;
  uint32_t radSumResetInterval;
  uint32_t minWateringInterval;
  uint32_t maxWateringInterval;
  uint32_t forcedWateringDuration;
  int prevWateringMode;
  uint8_t morningWateringCount;
  uint32_t morningWateringInterval;
  uint32_t morningWateringDuration;
  uint32_t radCheckInterval;
  String windowCommand;
  uint32_t windowDuration;
  float radSum;
  uint8_t cycleCount;
  uint32_t lastWateringStartUnix;
  uint32_t lastSunTimeUnix;
  uint32_t lastManualWateringTimeUnix;
  bool morningStartedToday;
  bool morningWateringActive;
  uint8_t currentMorningWatering;
  uint32_t lastMorningWateringStartUnix;
  uint32_t lastMorningWateringEndUnix;
  bool pendingMorningComplete;
  uint32_t forceWateringEndTimeUnix;
  uint32_t hydroStartUnix;
  bool forcedWateringPerformed; // Новое: флаг, что в forced был реальный полив (pumpState=true)
} sett;
typedef struct {
  float temperature;
  float humidity;
} SensorData;
typedef struct {
  float temperature;
  float humidity;
  float ec;
  float ph;
} MatSensorData;
int windowPos = 0;
bool windowMoving = false;
String windowDirection = "";
uint32_t windowStartTime = 0;
int targetWindowPos = 0;
uint32_t moveDuration = 0;
unsigned long lastSaveTime = 0;
const unsigned long saveCooldown = 5000;
AsyncWebServer server(80);
float mapFloat(float x, float inMin, float inMax, float outMin, float outMax) {
  if (inMax == inMin) return outMin;
  return constrain((x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin, outMin, outMax);
}
bool isNight() {
  DateTime now = rtc.now();
  int h = now.hour();
  return h < sett.dayStart || h >= sett.nightStart;
}
float calcFogDuration(float hum, float temp) {
  float humDuration = mapFloat(hum, sett.fogMinHum, sett.fogMaxHum, sett.fogMaxDuration, sett.fogMinDuration);
  float tempDuration = mapFloat(temp, sett.fogMinTemp, sett.fogMaxTemp, sett.fogMinDuration, sett.fogMaxDuration);
  return max(humDuration, tempDuration);
}
float calcFogInterval(float hum, float temp) {
  float humInterval = mapFloat(hum, sett.fogMinHum, sett.fogMaxHum, sett.fogMinInterval, sett.fogMaxInterval);
  float tempInterval = mapFloat(temp, sett.fogMinTemp, sett.fogMaxTemp, sett.fogMaxInterval, sett.fogMinInterval);
  return min(humInterval, tempInterval);
}
void publishAllSensors() {
  if (!client.connected()) return;
  DynamicJsonDocument doc(4096);
  float avgTemp = 0, avgHum = 0, level = 0, wind = 0, sol = 0, pyrano = 0;
  SensorData outdoor = { 0, 0 };
  MatSensorData matSensor = { 0, 0, 0, 0 };
  SensorData avgData;
  if (xQueuePeek(qAvg, &avgData, 0) == pdTRUE) {
    avgTemp = avgData.temperature;
    avgHum = avgData.humidity;
  }
  xQueuePeek(qSol, &sol, 0);
  xQueuePeek(qLevel, &level, 0);
  xQueuePeek(qWind, &wind, 0);
  xQueuePeek(qOutdoor, &outdoor, 0);
  xQueuePeek(qMat, &matSensor, 0);
  xQueuePeek(qPyrano, &pyrano, 0);
  DateTime now = rtc.now();
  doc["timestamp"] = now.unixtime() - 10800;
  doc["temperature"] = avgTemp;
  doc["humidity"] = avgHum;
  doc["water_level"] = level;
  doc["wind_speed"] = wind;
  doc["pyrano"] = isnan(pyrano) ? 0 : pyrano;
  doc["solution_temperature"] = sol;
  doc["outdoor_temperature"] = outdoor.temperature;
  doc["outdoor_humidity"] = outdoor.humidity;
  doc["mat_temperature"] = matSensor.temperature;
  doc["mat_hum"] = matSensor.humidity;
  doc["mat_ec"] = matSensor.ec;
  doc["mat_ph"] = matSensor.ph;
  doc["window_position"] = windowPos;
  doc["fan_state"] = sett.fanState;
  doc["heat_state"] = sett.heatState;
  doc["pump_state"] = sett.pumpState;
  doc["fog_state"] = sett.fogState;
  doc["sol_heat_state"] = sett.solHeatState;
  doc["hydro_mix"] = sett.hydroMix;
  doc["fill_state"] = sett.fillState;
  doc["watering_mode"] = sett.wateringMode;
  doc["rad_sum"] = sett.radSum;
  doc["cycle_count"] = sett.cycleCount;
  doc["last_watering_start_unix"] = sett.lastWateringStartUnix;
  doc["force_watering_active"] = sett.forceWateringActive;
  doc["force_watering_override"] = sett.forceWateringOverride;
  doc["force_watering_end_time_unix"] = sett.forceWateringEndTimeUnix;
  String json;
  serializeJson(doc, json);
  client.publish("greenhouse/esp32/all_sensors", json.c_str(), true);
}
void addToHistory(float temp, float hum) {
  float level, wind, outdoorTemp, outdoorHum;
  xQueuePeek(qLevel, &level, 0);
  xQueuePeek(qWind, &wind, 0);
  SensorData outdoor;
  xQueuePeek(qOutdoor, &outdoor, 0);
  outdoorTemp = outdoor.temperature;
  outdoorHum = outdoor.humidity;
  history[historyIndex] = {
    millis(),
    temp,
    hum,
    level,
    wind,
    outdoorTemp,
    outdoorHum,
    (float)windowPos,
    sett.fanState,
    sett.heatState,
    sett.pumpState,
    sett.solHeatState,
    sett.hydroMix,
    sett.fillState,
    sett.fogState
  };
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  saveHistory();
}
void applyDefaultSettings() {
  Serial.println("applyDefaultSettings: Applying factory defaults due to corrupted/zeroed settings.");
  sett.minTemp = 22.0;
  sett.maxTemp = 26.0;
  sett.minHum = 95.0;
  sett.maxHum = 99.0;
  sett.openTime = 156000UL;
  sett.closeTime = 156000UL;
  sett.heatOn = 19.0;
  sett.heatOff = 21.0;
  sett.fanInterval = 30UL * 60 * 1000;
  sett.fanDuration = 5UL * 60 * 1000;
  sett.fogMorningStart = 7;
  sett.fogMorningEnd = 8;
  sett.fogDayStart = 8;
  sett.fogDayEnd = 16;
  sett.fogMorningDuration = 10UL * 1000;
  sett.fogMorningInterval = 30UL * 60 * 1000;
  sett.fogDayDuration = 10UL * 1000;
  sett.fogDayInterval = 2UL * 60 * 1000;
  sett.fogDelay = 4;
  sett.fogMinHum = 30.0;
  sett.fogMaxHum = 85.0;
  sett.fogMinTemp = 20.0;
  sett.fogMaxTemp = 32.0;
  sett.fogMinDuration = 5UL * 1000;
  sett.fogMaxDuration = 30UL * 1000;
  sett.fogMinInterval = 1UL * 60 * 1000;
  sett.fogMaxInterval = 2UL * 60 * 60 * 1000;
  sett.solOn = 18.0;
  sett.solOff = 22.0;
  sett.wind1 = 10.0;
  sett.wind2 = 14.0;
  sett.windLockMinutes = 5;
  sett.nightStart = 17;
  sett.dayStart = 7;
  sett.nightOffset = 3.0;
  sett.manualWindow = false;
  sett.manualHeat = false;
  sett.manualPump = false;
  sett.manualSol = false;
  sett.fogMode = 0;
  sett.manualFan = false;
  sett.hydroMixDuration = 10UL * 60 * 1000;
  sett.previousManualPump = false;
  sett.manualPumpOverride = false;
  sett.forceFogOn = false;
  sett.autoFogDayStart = 10;
  sett.autoFogDayEnd = 17;
  sett.levelMin = 1.0;
  sett.levelMax = 1665.0;
  sett.heatState = false;
  sett.fanState = false;
  sett.fogState = false;
  sett.pumpState = false;
  sett.solHeatState = false;
  sett.hydroMix = false;
  sett.hydroStart = 0;
  sett.fillState = false;
  for (int i = 0; i < 4; i++) {
    sett.wateringModes[i].enabled = false;
    sett.wateringModes[i].startHour = 0;
    sett.wateringModes[i].endHour = 0;
    sett.wateringModes[i].duration = 0;
  }
  sett.forceWateringActive = false;
  sett.forceWateringOverride = false;
  sett.forceWateringEndTime = 0;
  sett.wateringMode = 0;
  sett.manualWateringInterval = 30UL * 60 * 1000;
  sett.manualWateringDuration = 120000UL;
  sett.matMinHumidity = 40.0;
  sett.matMaxEC = 9.0;
  sett.radThreshold = 1.5;
  sett.pumpTime = 18000UL;
  sett.wateringStartHour = 9;
  sett.wateringStartMinute = 0;
  sett.wateringEndHour = 17;
  sett.wateringEndMinute = 0;
  sett.maxWateringCycles = 15;
  sett.radSumResetInterval = 30UL * 60 * 1000;
  sett.minWateringInterval = 30UL * 60 * 1000;
  sett.maxWateringInterval = 60UL * 60 * 1000;
  sett.forcedWateringDuration = 30000UL;
  sett.prevWateringMode = 0;
  sett.morningWateringCount = 1;
  sett.morningWateringInterval = 40UL * 60 * 1000;
  sett.morningWateringDuration = 180UL * 1000;
  sett.radCheckInterval = 60UL * 1000;
  sett.radSum = 0.0f;
  sett.cycleCount = 0;
  sett.lastWateringStartUnix = 0;
  sett.lastSunTimeUnix = 0;
  sett.lastManualWateringTimeUnix = 0;
  sett.morningStartedToday = false;
  sett.morningWateringActive = false;
  sett.currentMorningWatering = 0;
  sett.lastMorningWateringStartUnix = 0;
  sett.lastMorningWateringEndUnix = 0;
  sett.pendingMorningComplete = false;
  sett.forceWateringEndTimeUnix = 0;
  sett.hydroStartUnix = 0;
  sett.forcedWateringPerformed = false;
  Serial.println("applyDefaultSettings: Defaults applied.");
}
bool isSettingsZeroed() {
  if (sett.minTemp > 0 || sett.maxTemp > 0 || sett.minHum > 0 || sett.maxHum > 0 || sett.openTime > 0 || sett.heatOn > 0 || sett.fanInterval > 0 || sett.radThreshold > 0 || sett.pumpTime > 0 || sett.wateringStartHour > 0 || sett.dayStart > 0 || sett.levelMin > 0) {
    return false;
  }
  Serial.println("isSettingsZeroed: Detected zeroed/corrupted settings.");
  return true;
}
void loadSettings() {
  Serial.println("loadSettings: Attempting to open /settings.json for reading...");
  File f = LittleFS.open("/settings.json", "r");
  if (!f) {
    Serial.println("loadSettings: Failed to open /settings.json for reading. File may not exist or is inaccessible. Using defaults.");
    Serial.printf("loadSettings: Current/Default openTime = %u, closeTime = %u, manualPump = %s\n", sett.openTime, sett.closeTime, sett.manualPump ? "true" : "false");
    applyDefaultSettings();
    saveSettings("load_failure");
    return;
  }
  Serial.println("loadSettings: File opened successfully.");
  size_t size = f.size();
  Serial.printf("loadSettings: File size is %d bytes.\n", size);
  if (size == 0) {
    Serial.println("loadSettings: File is empty. Using defaults.");
    f.close();
    applyDefaultSettings();
    saveSettings("empty_file");
    return;
  }
  size_t jsonSize = size + 512;
  Serial.printf("loadSettings: Allocating DynamicJsonDocument with size %d\n", jsonSize);
  DynamicJsonDocument doc(jsonSize);
  Serial.println("loadSettings: Attempting to deserialize JSON...");
  DeserializationError error = deserializeJson(doc, f);
  if (error) {
    Serial.print("loadSettings: Failed to parse settings file. Error: ");
    Serial.println(error.c_str());
    f.close();
    applyDefaultSettings();
    saveSettings("parse_error");
    return;
  }
  Serial.println("loadSettings: JSON parsed successfully.");
  f.close();
  Serial.println("loadSettings: Loading individual settings from JSON to 'sett' structure...");
  sett.minTemp = doc["minTemp"] | 20;
  sett.maxTemp = doc["maxTemp"] | 28;
  sett.minHum = doc["minHum"] | 50;
  sett.maxHum = doc["maxHum"] | 90;
  sett.openTime = doc["openTime"] | 10000;
  sett.closeTime = doc["closeTime"] | 10000;
  sett.heatOn = doc["heatOn"] | 18;
  sett.heatOff = doc["heatOff"] | 22;
  sett.fanInterval = doc["fanInterval"] | (30 * 60 * 1000UL);
  sett.fanDuration = doc["fanDuration"] | (5 * 60 * 1000UL);
  sett.fogMorningStart = doc["fogMorningStart"] | 7;
  sett.fogMorningEnd = doc["fogMorningEnd"] | 8;
  sett.fogDayStart = doc["fogDayStart"] | 8;
  sett.fogDayEnd = doc["fogDayEnd"] | 16;
  sett.fogMorningDuration = doc["fogMorningDuration"] | (10 * 1000UL);
  sett.fogMorningInterval = doc["fogMorningInterval"] | (30 * 60 * 1000UL);
  sett.fogDayDuration = doc["fogDayDuration"] | (10 * 1000UL);
  sett.fogDayInterval = doc["fogDayInterval"] | (2 * 60 * 1000UL);
  sett.fogDelay = doc["fogDelay"] | (30 * 1000UL);
  sett.fogMinHum = doc["fogMinHum"] | 60;
  sett.fogMaxHum = doc["fogMaxHum"] | 95;
  sett.fogMinTemp = doc["fogMinTemp"] | 18;
  sett.fogMaxTemp = doc["fogMaxTemp"] | 30;
  sett.fogMinDuration = doc["fogMinDuration"] | (5 * 1000UL);
  sett.fogMaxDuration = doc["fogMaxDuration"] | (30 * 1000UL);
  sett.fogMinInterval = doc["fogMinInterval"] | (10 * 60 * 1000UL);
  sett.fogMaxInterval = doc["fogMaxInterval"] | (2 * 60 * 60 * 1000UL);
  sett.solOn = doc["solOn"] | 18;
  sett.solOff = doc["solOff"] | 22;
  sett.wind1 = doc["wind1"] | 5;
  sett.wind2 = doc["wind2"] | 10;
  sett.windLockMinutes = doc["windLockMinutes"] | 5;
  sett.nightStart = doc["nightStart"] | 22;
  sett.dayStart = doc["dayStart"] | 6;
  sett.nightOffset = doc["nightOffset"] | 2;
  sett.manualWindow = doc["manualWindow"] | false;
  sett.manualHeat = doc["manualHeat"] | false;
  sett.manualPump = doc["manualPump"] | false;
  sett.manualSol = doc["manualSol"] | false;
  sett.fogMode = doc["fogMode"] | 0;
  sett.manualFan = doc["manualFan"] | false;
  sett.hydroMixDuration = doc["hydroMixDuration"] | (10 * 60 * 1000UL);
  sett.previousManualPump = doc["previousManualPump"] | false;
  sett.manualPumpOverride = doc["manualPumpOverride"] | false;
  sett.forceFogOn = doc["forceFogOn"] | false;
  sett.autoFogDayStart = doc["autoFogDayStart"] | 10;
  sett.autoFogDayEnd = doc["autoFogDayEnd"] | 17;
  sett.levelMin = doc["levelMin"] | 10;
  sett.levelMax = doc["levelMax"] | 90;
  sett.heatState = doc["heatState"] | false;
  sett.fanState = doc["fanState"] | false;
  sett.fogState = doc["fogState"] | false;
  sett.pumpState = doc["pumpState"] | false;
  sett.solHeatState = doc["solHeatState"] | false;
  sett.hydroMix = doc["hydroMix"] | false;
  sett.hydroStart = doc["hydroStart"] | 0;
  sett.fillState = doc["fillState"] | false;
  JsonArray wmArray = doc["wateringModes"];
  if (wmArray.size() == 4) {
    for (int i = 0; i < 4; i++) {
      JsonObject wm = wmArray[i];
      sett.wateringModes[i].enabled = wm["enabled"] | false;
      sett.wateringModes[i].startHour = wm["startHour"] | 0;
      sett.wateringModes[i].endHour = wm["endHour"] | 0;
      sett.wateringModes[i].duration = wm["duration"] | 0;
    }
  } else {
    for (int i = 0; i < 4; i++) {
      sett.wateringModes[i].enabled = false;
      sett.wateringModes[i].startHour = 0;
      sett.wateringModes[i].endHour = 0;
      sett.wateringModes[i].duration = 0;
    }
  }
  sett.pumpTime = doc["pumpTime"] | 30000UL;
  sett.wateringMode = doc["wateringMode"] | 0;
  sett.manualWateringInterval = doc["manualWateringInterval"] | (60 * 60 * 1000UL);
  sett.manualWateringDuration = doc["manualWateringDuration"] | 30000UL;
  sett.matMinHumidity = doc["matMinHumidity"] | 40.0;
  sett.matMaxEC = doc["matMaxEC"] | 3.0;
  sett.radThreshold = doc["radThreshold"] | 10.0;
  sett.forceWateringActive = doc["forceWateringActive"] | false;
  sett.forceWateringOverride = doc["forceWateringOverride"] | false;
  sett.forceWateringEndTime = doc["forceWateringEndTime"] | 0UL;
  sett.wateringStartHour = doc["wateringStartHour"] | 9;
  sett.wateringStartMinute = doc["wateringStartMinute"] | 0;
  sett.wateringEndHour = doc["wateringEndHour"] | 17;
  sett.wateringEndMinute = doc["wateringEndMinute"] | 0;
  sett.maxWateringCycles = doc["maxWateringCycles"] | 15;
  sett.radSumResetInterval = doc["radSumResetInterval"] | (30 * 60 * 1000UL);
  sett.minWateringInterval = doc["minWateringInterval"] | (60 * 60 * 1000UL);
  sett.maxWateringInterval = doc["maxWateringInterval"] | (90UL * 60 * 1000);
  sett.forcedWateringDuration = doc["forcedWateringDuration"] | 30000UL;
  sett.prevWateringMode = doc["prevWateringMode"] | 0;
  sett.morningWateringCount = doc["morningWateringCount"] | 0;
  sett.morningWateringInterval = doc["morningWateringInterval"] | (40 * 60 * 1000UL);
  sett.morningWateringDuration = doc["morningWateringDuration"] | (180 * 1000UL);
  sett.radCheckInterval = doc["radCheckInterval"] | (60 * 1000UL);
  if (sett.radCheckInterval == 0) {
    sett.radCheckInterval = 60 * 1000UL;
  }
  sett.radSum = doc["radSum"] | 0.0f;
  sett.cycleCount = doc["cycleCount"] | 0;
  sett.lastWateringStartUnix = doc["lastWateringStartUnix"] | 0UL;
  sett.lastSunTimeUnix = doc["lastSunTimeUnix"] | 0UL;
  sett.lastManualWateringTimeUnix = doc["lastManualWateringTimeUnix"] | 0UL;
  sett.morningStartedToday = doc["morningStartedToday"] | false;
  sett.morningWateringActive = doc["morningWateringActive"] | false;
  sett.currentMorningWatering = doc["currentMorningWatering"] | 0;
  sett.lastMorningWateringStartUnix = doc["lastMorningWateringStartUnix"] | 0UL;
  sett.lastMorningWateringEndUnix = doc["lastMorningWateringEndUnix"] | 0UL;
  sett.pendingMorningComplete = doc["pendingMorningComplete"] | false;
  sett.forceWateringEndTimeUnix = doc["forceWateringEndTimeUnix"] | 0UL;
  sett.hydroStartUnix = doc["hydroStartUnix"] | 0UL;
  sett.forcedWateringPerformed = doc["forcedWateringPerformed"] | false;
  if (isSettingsZeroed()) {
    applyDefaultSettings();
    saveSettings("zeroed_recovery");
  } else {
    Serial.println("loadSettings: File closed. Settings loading process finished.");
    Serial.println("loadSettings: Settings loaded successfully (not zeroed).");
  }
}
void publishSettings() {
  DynamicJsonDocument doc(5120);
  doc["minTemp"] = sett.minTemp;
  doc["maxTemp"] = sett.maxTemp;
  doc["minHum"] = sett.minHum;
  doc["maxHum"] = sett.maxHum;
  doc["openTime"] = sett.openTime;
  doc["closeTime"] = sett.closeTime;
  doc["heatOn"] = sett.heatOn;
  doc["heatOff"] = sett.heatOff;
  doc["fanInterval"] = sett.fanInterval;
  doc["fanDuration"] = sett.fanDuration;
  doc["fogMorningStart"] = sett.fogMorningStart;
  doc["fogMorningEnd"] = sett.fogMorningEnd;
  doc["fogDayStart"] = sett.fogDayStart;
  doc["fogDayEnd"] = sett.fogDayEnd;
  doc["fogMorningDuration"] = sett.fogMorningDuration;
  doc["fogMorningInterval"] = sett.fogMorningInterval;
  doc["fogDayDuration"] = sett.fogDayDuration;
  doc["fogDayInterval"] = sett.fogDayInterval;
  doc["fogDelay"] = sett.fogDelay;
  doc["fogMinHum"] = sett.fogMinHum;
  doc["fogMaxHum"] = sett.fogMaxHum;
  doc["fogMinTemp"] = sett.fogMinTemp;
  doc["fogMaxTemp"] = sett.fogMaxTemp;
  doc["fogMinDuration"] = sett.fogMinDuration;
  doc["fogMaxDuration"] = sett.fogMaxDuration;
  doc["fogMinInterval"] = sett.fogMinInterval;
  doc["fogMaxInterval"] = sett.fogMaxInterval;
  doc["solOn"] = sett.solOn;
  doc["solOff"] = sett.solOff;
  doc["wind1"] = sett.wind1;
  doc["wind2"] = sett.wind2;
  doc["windLockMinutes"] = sett.windLockMinutes;
  doc["nightStart"] = sett.nightStart;
  doc["dayStart"] = sett.dayStart;
  doc["nightOffset"] = sett.nightOffset;
  doc["manualWindow"] = sett.manualWindow;
  doc["manualHeat"] = sett.manualHeat;
  doc["manualPump"] = sett.manualPump;
  doc["manualSol"] = sett.manualSol;
  doc["fogMode"] = sett.fogMode;
  doc["manualFan"] = sett.manualFan;
  doc["hydroMixDuration"] = sett.hydroMixDuration;
  doc["previousManualPump"] = sett.previousManualPump;
  doc["manualPumpOverride"] = sett.manualPumpOverride;
  doc["forceFogOn"] = sett.forceFogOn;
  doc["heatState"] = sett.heatState;
  doc["fanState"] = sett.fanState;
  doc["fogState"] = sett.fogState;
  doc["pumpState"] = sett.pumpState;
  doc["solHeatState"] = sett.solHeatState;
  doc["autoFogDayStart"] = sett.autoFogDayStart;
  doc["autoFogDayEnd"] = sett.autoFogDayEnd;
  doc["levelMin"] = sett.levelMin;
  doc["levelMax"] = sett.levelMax;
  doc["hydroMix"] = sett.hydroMix;
  doc["hydroStart"] = sett.hydroStart;
  doc["fillState"] = sett.fillState;
  JsonArray modes = doc.createNestedArray("wateringModes");
  for (int i = 0; i < 4; i++) {
    JsonObject mode = modes.createNestedObject();
    mode["enabled"] = sett.wateringModes[i].enabled;
    mode["startHour"] = sett.wateringModes[i].startHour;
    mode["endHour"] = sett.wateringModes[i].endHour;
    mode["duration"] = sett.wateringModes[i].duration;
  }
  doc["radThreshold"] = sett.radThreshold;
  doc["pumpTime"] = sett.pumpTime;
  doc["wateringStartHour"] = sett.wateringStartHour;
  doc["wateringStartMinute"] = sett.wateringStartMinute;
  doc["wateringEndHour"] = sett.wateringEndHour;
  doc["wateringEndMinute"] = sett.wateringEndMinute;
  doc["maxWateringCycles"] = sett.maxWateringCycles;
  doc["radSumResetInterval"] = sett.radSumResetInterval;
  doc["minWateringInterval"] = sett.minWateringInterval;
  doc["wateringMode"] = sett.wateringMode;
  doc["manualWateringInterval"] = sett.manualWateringInterval;
  doc["manualWateringDuration"] = sett.manualWateringDuration;
  doc["matMinHumidity"] = sett.matMinHumidity;
  doc["matMaxEC"] = sett.matMaxEC;
  doc["maxWateringInterval"] = sett.maxWateringInterval;
  doc["forcedWateringDuration"] = sett.forcedWateringDuration;
  doc["prevWateringMode"] = sett.prevWateringMode;
  doc["morningWateringCount"] = sett.morningWateringCount;
  doc["morningWateringInterval"] = sett.morningWateringInterval;
  doc["morningWateringDuration"] = sett.morningWateringDuration;
  doc["radSum"] = sett.radSum;
  doc["cycleCount"] = sett.cycleCount;
  doc["lastWateringStartUnix"] = sett.lastWateringStartUnix;
  doc["lastSunTimeUnix"] = sett.lastSunTimeUnix;
  doc["lastManualWateringTimeUnix"] = sett.lastManualWateringTimeUnix;
  doc["morningStartedToday"] = sett.morningStartedToday;
  doc["morningWateringActive"] = sett.morningWateringActive;
  doc["currentMorningWatering"] = sett.currentMorningWatering;
  doc["lastMorningWateringStartUnix"] = sett.lastMorningWateringStartUnix;
  doc["lastMorningWateringEndUnix"] = sett.lastMorningWateringEndUnix;
  doc["pendingMorningComplete"] = sett.pendingMorningComplete;
  doc["forceWateringEndTimeUnix"] = sett.forceWateringEndTimeUnix;
  doc["hydroStartUnix"] = sett.hydroStartUnix;
  doc["forcedWateringPerformed"] = sett.forcedWateringPerformed;
  String json;
  serializeJson(doc, json);
  client.publish("greenhouse/esp32/settings_full", json.c_str(), true);
  Serial.println("Settings published to greenhouse/esp32/settings_full");
}
void saveSettings(const char *caller) {
  if (millis() - lastSaveTime < saveCooldown) {
    Serial.printf("saveSettings: Called by %s, but save cooldown (%ums) not expired. Skipping save.\n", caller ? caller : "Unknown", saveCooldown);
    return;
  }
  lastSaveTime = millis();
  if (isSettingsZeroed()) {
    Serial.printf("saveSettings: Settings are zeroed, applying defaults before save by %s\n", caller ? caller : "Unknown");
    applyDefaultSettings();
  }
  File file = LittleFS.open("/settings.json", "w");
  if (!file) {
    Serial.println("Failed to open settings file for writing");
    return;
  }
  DynamicJsonDocument doc(5120);
  doc["minTemp"] = sett.minTemp;
  doc["maxTemp"] = sett.maxTemp;
  doc["minHum"] = sett.minHum;
  doc["maxHum"] = sett.maxHum;
  doc["openTime"] = sett.openTime;
  doc["closeTime"] = sett.closeTime;
  doc["heatOn"] = sett.heatOn;
  doc["heatOff"] = sett.heatOff;
  doc["fanInterval"] = sett.fanInterval;
  doc["fanDuration"] = sett.fanDuration;
  doc["fogMorningStart"] = sett.fogMorningStart;
  doc["fogMorningEnd"] = sett.fogMorningEnd;
  doc["fogDayStart"] = sett.fogDayStart;
  doc["fogDayEnd"] = sett.fogDayEnd;
  doc["fogMorningDuration"] = sett.fogMorningDuration;
  doc["fogMorningInterval"] = sett.fogMorningInterval;
  doc["fogDayDuration"] = sett.fogDayDuration;
  doc["fogDayInterval"] = sett.fogDayInterval;
  doc["fogDelay"] = sett.fogDelay;
  doc["fogMinHum"] = sett.fogMinHum;
  doc["fogMaxHum"] = sett.fogMaxHum;
  doc["fogMinTemp"] = sett.fogMinTemp;
  doc["fogMaxTemp"] = sett.fogMaxTemp;
  doc["fogMinDuration"] = sett.fogMinDuration;
  doc["fogMaxDuration"] = sett.fogMaxDuration;
  doc["fogMinInterval"] = sett.fogMinInterval;
  doc["fogMaxInterval"] = sett.fogMaxInterval;
  doc["solOn"] = sett.solOn;
  doc["solOff"] = sett.solOff;
  doc["wind1"] = sett.wind1;
  doc["wind2"] = sett.wind2;
  doc["windLockMinutes"] = sett.windLockMinutes;
  doc["nightStart"] = sett.nightStart;
  doc["dayStart"] = sett.dayStart;
  doc["nightOffset"] = sett.nightOffset;
  doc["manualWindow"] = sett.manualWindow;
  doc["manualHeat"] = sett.manualHeat;
  doc["manualPump"] = sett.manualPump;
  doc["manualSol"] = sett.manualSol;
  doc["fogMode"] = sett.fogMode;
  doc["manualFan"] = sett.manualFan;
  doc["hydroMixDuration"] = sett.hydroMixDuration;
  doc["previousManualPump"] = sett.previousManualPump;
  doc["manualPumpOverride"] = sett.manualPumpOverride;
  doc["forceFogOn"] = sett.forceFogOn;
  doc["autoFogDayStart"] = sett.autoFogDayStart;
  doc["autoFogDayEnd"] = sett.autoFogDayEnd;
  doc["levelMin"] = sett.levelMin;
  doc["levelMax"] = sett.levelMax;
  doc["heatState"] = sett.heatState;
  doc["fanState"] = sett.fanState;
  doc["fogState"] = sett.fogState;
  doc["pumpState"] = sett.pumpState;
  doc["solHeatState"] = sett.solHeatState;
  doc["hydroMix"] = sett.hydroMix;
  doc["hydroStart"] = sett.hydroStart;
  doc["fillState"] = sett.fillState;
  JsonArray wmArray = doc.createNestedArray("wateringModes");
  for (int i = 0; i < 4; i++) {
    JsonObject wm = wmArray.createNestedObject();
    wm["enabled"] = sett.wateringModes[i].enabled;
    wm["startHour"] = sett.wateringModes[i].startHour;
    wm["endHour"] = sett.wateringModes[i].endHour;
    wm["duration"] = sett.wateringModes[i].duration;
  }
  doc["wateringMode"] = sett.wateringMode;
  doc["manualWateringInterval"] = sett.manualWateringInterval;
  doc["manualWateringDuration"] = sett.manualWateringDuration;
  doc["matMinHumidity"] = sett.matMinHumidity;
  doc["matMaxEC"] = sett.matMaxEC;
  doc["radThreshold"] = sett.radThreshold;
  doc["forceWateringActive"] = sett.forceWateringActive;
  doc["forceWateringOverride"] = sett.forceWateringOverride;
  doc["forceWateringEndTime"] = sett.forceWateringEndTime;
  doc["pumpTime"] = sett.pumpTime;
  doc["wateringStartHour"] = sett.wateringStartHour;
  doc["wateringStartMinute"] = sett.wateringStartMinute;
  doc["wateringEndHour"] = sett.wateringEndHour;
  doc["wateringEndMinute"] = sett.wateringEndMinute;
  doc["maxWateringCycles"] = sett.maxWateringCycles;
  doc["radSumResetInterval"] = sett.radSumResetInterval;
  doc["minWateringInterval"] = sett.minWateringInterval;
  doc["maxWateringInterval"] = sett.maxWateringInterval;
  doc["forcedWateringDuration"] = sett.forcedWateringDuration;
  doc["prevWateringMode"] = sett.prevWateringMode;
  doc["morningWateringCount"] = sett.morningWateringCount;
  doc["morningWateringInterval"] = sett.morningWateringInterval;
  doc["morningWateringDuration"] = sett.morningWateringDuration;
  doc["radCheckInterval"] = sett.radCheckInterval;
  doc["radSum"] = sett.radSum;
  doc["cycleCount"] = sett.cycleCount;
  doc["lastWateringStartUnix"] = sett.lastWateringStartUnix;
  doc["lastSunTimeUnix"] = sett.lastSunTimeUnix;
  doc["lastManualWateringTimeUnix"] = sett.lastManualWateringTimeUnix;
  doc["morningStartedToday"] = sett.morningStartedToday;
  doc["morningWateringActive"] = sett.morningWateringActive;
  doc["currentMorningWatering"] = sett.currentMorningWatering;
  doc["lastMorningWateringStartUnix"] = sett.lastMorningWateringStartUnix;
  doc["lastMorningWateringEndUnix"] = sett.lastMorningWateringEndUnix;
  doc["pendingMorningComplete"] = sett.pendingMorningComplete;
  doc["forceWateringEndTimeUnix"] = sett.forceWateringEndTimeUnix;
  doc["hydroStartUnix"] = sett.hydroStartUnix;
  doc["forcedWateringPerformed"] = sett.forcedWateringPerformed;
  size_t bytesWritten = serializeJson(doc, file);
  if (bytesWritten == 0) {
    Serial.println("Failed to write to settings file (serializeJson returned 0)");
  } else {
    Serial.printf("Settings saved to LittleFS by %s. Bytes written: %d\n", caller ? caller : "Unknown", bytesWritten);
  }
  file.close();
}
void publishHistory() {
  if (client.connected()) {
    DynamicJsonDocument doc(4096);
    JsonArray historyArray = doc.createNestedArray("history");
    for (int i = 0; i < HISTORY_SIZE; i++) {
      JsonObject entry = historyArray.createNestedObject();
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
    client.publish("greenhouse/esp32/history", json.c_str(), true);
    Serial.println("Data published to greenhouse/esp32/history");
  }
}
void saveHistory() {
  Serial.println("saveHistory: Saving history to /history.json");
  File file = LittleFS.open("/history.json", "w");
  if (!file) {
    Serial.println("saveHistory: Failed to open /history.json for writing");
    return;
  }
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.createNestedArray("history");
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (history[i].timestamp == 0) continue;
    JsonObject obj = arr.createNestedObject();
    obj["timestamp"] = history[i].timestamp;
    obj["avgTemp"] = history[i].avgTemp;
    obj["avgHum"] = history[i].avgHum;
    obj["level"] = history[i].level;
    obj["windSpeed"] = history[i].windSpeed;
    obj["outdoorTemp"] = history[i].outdoorTemp;
    obj["outdoorHum"] = history[i].outdoorHum;
    obj["windowPosition"] = history[i].windowPosition;
    obj["fanState"] = history[i].fanState;
    obj["heatingState"] = history[i].heatingState;
    obj["wateringState"] = history[i].wateringState;
    obj["solutionHeatingState"] = history[i].solutionHeatingState;
    obj["hydroState"] = history[i].hydroState;
    obj["fillState"] = history[i].fillState;
    obj["fogState"] = history[i].fogState;
  }
  if (serializeJson(doc, file) == 0) {
    Serial.println("saveHistory: Failed to write to /history.json");
  } else {
    Serial.println("saveHistory: History saved successfully");
  }
  file.close();
}
void loadHistory() {
  Serial.println("loadHistory: Attempting to load /history.json");
  File file = LittleFS.open("/history.json", "r");
  if (!file) {
    Serial.println("loadHistory: Failed to open /history.json, using empty history");
    return;
  }
  DynamicJsonDocument doc(file.size() + 512);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.printf("loadHistory: Failed to parse /history.json, error: %s\n", error.c_str());
    file.close();
    return;
  }
  JsonArray arr = doc["history"];
  historyIndex = 0;
  for (JsonObject obj : arr) {
    if (historyIndex >= HISTORY_SIZE) break;
    history[historyIndex] = {
      obj["timestamp"] | 0,
      obj["avgTemp"] | 0.0f,
      obj["avgHum"] | 0.0f,
      obj["level"] | 0.0f,
      obj["windSpeed"] | 0.0f,
      obj["outdoorTemp"] | 0.0f,
      obj["outdoorHum"] | 0.0f,
      obj["windowPosition"] | 0.0f,
      obj["fanState"] | false,
      obj["heatingState"] | false,
      obj["wateringState"] | false,
      obj["solutionHeatingState"] | false,
      obj["hydroState"] | false,
      obj["fillState"] | false,
      obj["fogState"] | false
    };
    historyIndex++;
  }
  historyIndex = historyIndex % HISTORY_SIZE;
  file.close();
  Serial.printf("loadHistory: Loaded %d history entries\n", historyIndex);
}
void resetForcedToNormal(int targetMode) {
  // Функция для явного перехода из forced в авто (0) или ручной (1)
  DateTime nowDt = rtc.now();
  uint32_t nowUnix = nowDt.unixtime();
  sett.forceWateringActive = false;
  sett.forceWateringOverride = false;
  sett.fillState = false;
  sett.hydroMix = false;
  sett.hydroStartUnix = 0;
  sett.pumpState = false;
  sett.forceWateringEndTimeUnix = 0;
  sett.wateringMode = targetMode;
  // НЕ сбрасываем lastWateringStartUnix и radSum здесь — только если был реальный полив в forced
  if (sett.forcedWateringPerformed) {
    sett.lastWateringStartUnix = nowUnix;
    if (targetMode == 0) {
      sett.radSum = 0.0f;
      sett.cycleCount++;
    } else if (targetMode == 1) {
      sett.lastManualWateringTimeUnix = nowUnix;
    }
    sett.forcedWateringPerformed = false; // Сброс флага после учёта
  }
  // Обработка morning
  if (sett.morningWateringActive) {
    sett.lastMorningWateringEndUnix = nowUnix;
    if (sett.pendingMorningComplete) {
      sett.currentMorningWatering++;
      sett.pendingMorningComplete = false;
    }
    if (sett.currentMorningWatering >= sett.morningWateringCount) {
      sett.morningWateringActive = false;
      // НЕ сбрасываем radSum после morning — копим дальше
      // Устанавливаем lastWateringStartUnix на время старта последнего morning для minInterval
      sett.lastWateringStartUnix = sett.lastMorningWateringStartUnix;
      if (targetMode == 1) sett.lastManualWateringTimeUnix = sett.lastMorningWateringStartUnix;
      LOG("DEBUG MORNING-WATER: Sequence completed after manual reset! lastWateringStart set to last morning start (unix %lu)\n", sett.lastWateringStartUnix);
    }
  }
  LOG("DEBUG FORCED-RESET: Explicit transition to mode %d, forced flags cleared (unix %lu, performed=%s)\n", targetMode, nowUnix, sett.forcedWateringPerformed ? "true" : "false");
}
void callback(char *topic, byte *message, unsigned int length) {
  char *msg_buffer = (char *)malloc(length + 1);
  if (msg_buffer == nullptr) {
    Serial.println("ERROR: Not enough memory to process MQTT message");
    return;
  }
  memcpy(msg_buffer, message, length);
  msg_buffer[length] = '\0';
  String msg = String(msg_buffer);
  free(msg_buffer);
  msg_buffer = nullptr;
  Serial.printf("Message arrived [%s]: %s\n", topic, msg.c_str());
  if (strcmp(topic, "greenhouse/cmd/set_settings") == 0) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
      Serial.print("Failed to parse set_settings JSON: ");
      Serial.println(error.c_str());
      return;
    }
    bool needSave = false;
    const char *source = "MQTT_set_settings";
    if (doc.containsKey("minTemp")) {
      float val = doc["minTemp"];
      if (sett.minTemp != val) {
        sett.minTemp = val;
        needSave = true;
        Serial.printf("DEBUG: Updated minTemp to %.1f\n", sett.minTemp);
      }
    }
    if (doc.containsKey("maxTemp")) {
      float val = doc["maxTemp"];
      if (sett.maxTemp != val) {
        sett.maxTemp = val;
        needSave = true;
        Serial.printf("DEBUG: Updated maxTemp to %.1f\n", sett.maxTemp);
      }
    }
    if (doc.containsKey("minHum")) {
      float val = doc["minHum"];
      if (sett.minHum != val) {
        sett.minHum = val;
        needSave = true;
        Serial.printf("DEBUG: Updated minHum to %.1f\n", sett.minHum);
      }
    }
    if (doc.containsKey("maxHum")) {
      float val = doc["maxHum"];
      if (sett.maxHum != val) {
        sett.maxHum = val;
        needSave = true;
        Serial.printf("DEBUG: Updated maxHum to %.1f\n", sett.maxHum);
      }
    }
    if (doc.containsKey("heatOn")) {
      float val = doc["heatOn"];
      if (sett.heatOn != val) {
        sett.heatOn = val;
        needSave = true;
        Serial.printf("DEBUG: Updated heatOn to %.1f\n", sett.heatOn);
      }
    }
    if (doc.containsKey("heatOff")) {
      float val = doc["heatOff"];
      if (sett.heatOff != val) {
        sett.heatOff = val;
        needSave = true;
        Serial.printf("DEBUG: Updated heatOff to %.1f\n", sett.heatOff);
      }
    }
    if (doc.containsKey("solOn")) {
      float val = doc["solOn"];
      if (sett.solOn != val) {
        sett.solOn = val;
        needSave = true;
        Serial.printf("DEBUG: Updated solOn to %.1f\n", sett.solOn);
      }
    }
    if (doc.containsKey("solOff")) {
      float val = doc["solOff"];
      if (sett.solOff != val) {
        sett.solOff = val;
        needSave = true;
        Serial.printf("DEBUG: Updated solOff to %.1f\n", sett.solOff);
      }
    }
    if (doc.containsKey("fanInterval")) {
      uint32_t val = doc["fanInterval"];
      if (sett.fanInterval != val) {
        sett.fanInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fanInterval to %u\n", sett.fanInterval);
      }
    }
    if (doc.containsKey("fanDuration")) {
      uint32_t val = doc["fanDuration"];
      if (sett.fanDuration != val) {
        sett.fanDuration = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fanDuration to %u\n", sett.fanDuration);
      }
    }
    if (doc.containsKey("fogMinDuration")) {
      uint32_t val = doc["fogMinDuration"];
      if (sett.fogMinDuration != val) {
        sett.fogMinDuration = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMinDuration to %u\n", sett.fogMinDuration);
      }
    }
    if (doc.containsKey("fogMaxDuration")) {
      uint32_t val = doc["fogMaxDuration"];
      if (sett.fogMaxDuration != val) {
        sett.fogMaxDuration = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMaxDuration to %u\n", sett.fogMaxDuration);
      }
    }
    if (doc.containsKey("fogMinInterval")) {
      uint32_t val = doc["fogMinInterval"];
      if (sett.fogMinInterval != val) {
        sett.fogMinInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMinInterval to %u\n", sett.fogMinInterval);
      }
    }
    if (doc.containsKey("fogMaxInterval")) {
      uint32_t val = doc["fogMaxInterval"];
      if (sett.fogMaxInterval != val) {
        sett.fogMaxInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMaxInterval to %u\n", sett.fogMaxInterval);
      }
    }
    if (doc.containsKey("fogDelay")) {
      uint32_t val = doc["fogDelay"];
      if (sett.fogDelay != val) {
        sett.fogDelay = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogDelay to %u\n", sett.fogDelay);
      }
    }
    if (doc.containsKey("fogDayDuration")) {
      uint32_t val = doc["fogDayDuration"];
      if (sett.fogDayDuration != val) {
        sett.fogDayDuration = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogDayDuration to %u\n", sett.fogDayDuration);
      }
    }
    if (doc.containsKey("fogDayInterval")) {
      uint32_t val = doc["fogDayInterval"];
      if (sett.fogDayInterval != val) {
        sett.fogDayInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogDayInterval to %u\n", sett.fogDayInterval);
      }
    }
    if (doc.containsKey("fogMorningDuration")) {
      uint32_t val = doc["fogMorningDuration"];
      if (sett.fogMorningDuration != val) {
        sett.fogMorningDuration = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMorningDuration to %u\n", sett.fogMorningDuration);
      }
    }
    if (doc.containsKey("fogMorningInterval")) {
      uint32_t val = doc["fogMorningInterval"];
      if (sett.fogMorningInterval != val) {
        sett.fogMorningInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMorningInterval to %u\n", sett.fogMorningInterval);
      }
    }
    if (doc.containsKey("fogMorningStart")) {
      int val = doc["fogMorningStart"];
      if (sett.fogMorningStart != val) {
        sett.fogMorningStart = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMorningStart to %d\n", sett.fogMorningStart);
      }
    }
    if (doc.containsKey("fogMorningEnd")) {
      int val = doc["fogMorningEnd"];
      if (sett.fogMorningEnd != val) {
        sett.fogMorningEnd = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMorningEnd to %d\n", sett.fogMorningEnd);
      }
    }
    if (doc.containsKey("fogDayStart")) {
      int val = doc["fogDayStart"];
      if (sett.fogDayStart != val) {
        sett.fogDayStart = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogDayStart to %d\n", sett.fogDayStart);
      }
    }
    if (doc.containsKey("fogDayEnd")) {
      int val = doc["fogDayEnd"];
      if (sett.fogDayEnd != val) {
        sett.fogDayEnd = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogDayEnd to %d\n", sett.fogDayEnd);
      }
    }
    if (doc.containsKey("autoFogDayStart")) {
      int val = doc["autoFogDayStart"];
      if (sett.autoFogDayStart != val) {
        sett.autoFogDayStart = val;
        needSave = true;
        Serial.printf("DEBUG: Updated autoFogDayStart to %d\n", sett.autoFogDayStart);
      }
    }
    if (doc.containsKey("autoFogDayEnd")) {
      int val = doc["autoFogDayEnd"];
      if (sett.autoFogDayEnd != val) {
        sett.autoFogDayEnd = val;
        needSave = true;
        Serial.printf("DEBUG: Updated autoFogDayEnd to %d\n", sett.autoFogDayEnd);
      }
    }
    if (doc.containsKey("fogMinHum")) {
      float val = doc["fogMinHum"];
      if (sett.fogMinHum != val) {
        sett.fogMinHum = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMinHum to %.1f\n", sett.fogMinHum);
      }
    }
    if (doc.containsKey("fogMaxHum")) {
      float val = doc["fogMaxHum"];
      if (sett.fogMaxHum != val) {
        sett.fogMaxHum = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMaxHum to %.1f\n", sett.fogMaxHum);
      }
    }
    if (doc.containsKey("fogMinTemp")) {
      float val = doc["fogMinTemp"];
      if (sett.fogMinTemp != val) {
        sett.fogMinTemp = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMinTemp to %.1f\n", sett.fogMinTemp);
      }
    }
    if (doc.containsKey("fogMaxTemp")) {
      float val = doc["fogMaxTemp"];
      if (sett.fogMaxTemp != val) {
        sett.fogMaxTemp = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMaxTemp to %.1f\n", sett.fogMaxTemp);
      }
    }
    if (doc.containsKey("fogMode")) {
      int val = doc["fogMode"];
      if (sett.fogMode != val) {
        sett.fogMode = val;
        needSave = true;
        Serial.printf("DEBUG: Updated fogMode to %d\n", sett.fogMode);
      }
    }
    if (doc.containsKey("forceFogOn")) {
      bool val = doc["forceFogOn"];
      if (sett.forceFogOn != val) {
        sett.forceFogOn = val;
        needSave = true;
        Serial.printf("DEBUG: Updated forceFogOn to %s\n", sett.forceFogOn ? "true" : "false");
      }
    }
    if (doc.containsKey("openTime")) {
      uint32_t val = doc["openTime"];
      if (sett.openTime != val) {
        sett.openTime = val;
        needSave = true;
        Serial.printf("DEBUG: Updated openTime to %u\n", sett.openTime);
      }
    }
    if (doc.containsKey("closeTime")) {
      uint32_t val = doc["closeTime"];
      if (sett.closeTime != val) {
        sett.closeTime = val;
        needSave = true;
        Serial.printf("DEBUG: Updated closeTime to %u\n", sett.closeTime);
      }
    }
    if (doc.containsKey("wind1")) {
      float val = doc["wind1"];
      if (sett.wind1 != val) {
        sett.wind1 = val;
        needSave = true;
        Serial.printf("DEBUG: Updated wind1 to %.1f\n", sett.wind1);
      }
    }
    if (doc.containsKey("wind2")) {
      float val = doc["wind2"];
      if (sett.wind2 != val) {
        sett.wind2 = val;
        needSave = true;
        Serial.printf("DEBUG: Updated wind2 to %.1f\n", sett.wind2);
      }
    }
    if (doc.containsKey("windLockMinutes")) {
      uint32_t val = doc["windLockMinutes"];
      if (sett.windLockMinutes != val) {
        sett.windLockMinutes = val;
        needSave = true;
        Serial.printf("DEBUG: Updated windLockMinutes to %u\n", sett.windLockMinutes);
      }
    }
    if (doc.containsKey("nightOffset")) {
      float val = doc["nightOffset"];
      if (sett.nightOffset != val) {
        sett.nightOffset = val;
        needSave = true;
        Serial.printf("DEBUG: Updated nightOffset to %.1f\n", sett.nightOffset);
      }
    }
    if (doc.containsKey("dayStart")) {
      uint32_t val = doc["dayStart"];
      if (sett.dayStart != val) {
        sett.dayStart = val;
        needSave = true;
        Serial.printf("DEBUG: Updated dayStart to %u\n", sett.dayStart);
      }
    }
    if (doc.containsKey("nightStart")) {
      uint32_t val = doc["nightStart"];
      if (sett.nightStart != val) {
        sett.nightStart = val;
        needSave = true;
        Serial.printf("DEBUG: Updated nightStart to %u\n", sett.nightStart);
      }
    }
    if (doc.containsKey("manualHeat")) {
      bool val = doc["manualHeat"];
      if (sett.manualHeat != val) {
        sett.manualHeat = val;
        needSave = true;
        Serial.printf("DEBUG: Updated manualHeat to %s\n", sett.manualHeat ? "true" : "false");
      }
    }
    if (doc.containsKey("manualSol")) {
      bool val = doc["manualSol"];
      if (sett.manualSol != val) {
        sett.manualSol = val;
        needSave = true;
        Serial.printf("DEBUG: Updated manualSol to %s\n", sett.manualSol ? "true" : "false");
      }
    }
    if (doc.containsKey("manualFan")) {
      bool val = doc["manualFan"];
      if (sett.manualFan != val) {
        sett.manualFan = val;
        needSave = true;
        Serial.printf("DEBUG: Updated manualFan to %s\n", sett.manualFan ? "true" : "false");
      }
    }
    if (doc.containsKey("manualWindow")) {
      bool val = doc["manualWindow"];
      if (sett.manualWindow != val) {
        sett.manualWindow = val;
        needSave = true;
        Serial.printf("DEBUG: Updated manualWindow to %s\n", sett.manualWindow ? "true" : "false");
      }
    }
    if (doc.containsKey("manualPump")) {
      bool val = doc["manualPump"];
      if (sett.manualPump != val) {
        sett.manualPump = val;
        needSave = true;
        Serial.printf("DEBUG: Updated manualPump to %s\n", sett.manualPump ? "true" : "false");
      }
    }
    if (doc.containsKey("radThreshold")) {
      float val = doc["radThreshold"];
      if (sett.radThreshold != val) {
        sett.radThreshold = val;
        needSave = true;
        Serial.printf("DEBUG: Updated radThreshold to %.2f\n", sett.radThreshold);
      }
    }
    if (doc.containsKey("pumpTime")) {
      uint32_t val = doc["pumpTime"];
      if (sett.pumpTime != val) {
        sett.pumpTime = val;
        needSave = true;
        Serial.printf("DEBUG: Updated pumpTime to %u\n", sett.pumpTime);
      }
    }
    if (doc.containsKey("wateringStartHour")) {
      uint8_t val = doc["wateringStartHour"];
      if (sett.wateringStartHour != val) {
        sett.wateringStartHour = val;
        needSave = true;
        Serial.printf("DEBUG: Updated wateringStartHour to %u\n", sett.wateringStartHour);
      }
    }
    if (doc.containsKey("wateringStartMinute")) {
      uint8_t val = doc["wateringStartMinute"];
      if (sett.wateringStartMinute != val) {
        sett.wateringStartMinute = val;
        needSave = true;
        Serial.printf("DEBUG: Updated wateringStartMinute to %u\n", sett.wateringStartMinute);
      }
    }
    if (doc.containsKey("wateringEndHour")) {
      uint8_t val = doc["wateringEndHour"];
      if (sett.wateringEndHour != val) {
        sett.wateringEndHour = val;
        needSave = true;
        Serial.printf("DEBUG: Updated wateringEndHour to %u\n", sett.wateringEndHour);
      }
    }
    if (doc.containsKey("wateringEndMinute")) {
      uint8_t val = doc["wateringEndMinute"];
      if (sett.wateringEndMinute != val) {
        sett.wateringEndMinute = val;
        needSave = true;
        Serial.printf("DEBUG: Updated wateringEndMinute to %u\n", sett.wateringEndMinute);
      }
    }
    if (doc.containsKey("maxWateringCycles")) {
      uint8_t val = doc["maxWateringCycles"];
      if (sett.maxWateringCycles != val) {
        sett.maxWateringCycles = val;
        needSave = true;
        Serial.printf("DEBUG: Updated maxWateringCycles to %u\n", sett.maxWateringCycles);
      }
    }
    if (doc.containsKey("radSumResetInterval")) {
      uint32_t val = doc["radSumResetInterval"];
      if (sett.radSumResetInterval != val) {
        sett.radSumResetInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated radSumResetInterval to %u\n", sett.radSumResetInterval);
      }
    }
    if (doc.containsKey("minWateringInterval")) {
      uint32_t val = doc["minWateringInterval"];
      if (sett.minWateringInterval != val) {
        sett.minWateringInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated minWateringInterval to %u\n", sett.minWateringInterval);
      }
    }
    if (doc.containsKey("maxWateringInterval")) {
      uint32_t val = doc["maxWateringInterval"];
      if (sett.maxWateringInterval != val) {
        sett.maxWateringInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated maxWateringInterval to %u ms\n", sett.maxWateringInterval);
      }
    }
    if (doc.containsKey("forcedWateringDuration")) {
      uint32_t val = doc["forcedWateringDuration"];
      if (sett.forcedWateringDuration != val) {
        sett.forcedWateringDuration = val;
        needSave = true;
        Serial.printf("DEBUG: Updated forcedWateringDuration to %u ms\n", sett.forcedWateringDuration);
      }
    }
    if (doc.containsKey("prevWateringMode")) {
      int val = doc["prevWateringMode"];
      if (sett.prevWateringMode != val) {
        sett.prevWateringMode = val;
        needSave = true;
        Serial.printf("DEBUG: Updated prevWateringMode to %d\n", sett.prevWateringMode);
      }
    }
    if (doc.containsKey("morningWateringCount")) {
      uint8_t val = doc["morningWateringCount"];
      if (sett.morningWateringCount != val) {
        sett.morningWateringCount = val;
        needSave = true;
        Serial.printf("DEBUG: Updated morningWateringCount to %u\n", sett.morningWateringCount);
      }
    }
    if (doc.containsKey("morningWateringInterval")) {
      uint64_t tempVal = (uint64_t)doc["morningWateringInterval"];
      uint32_t newVal = (uint32_t)tempVal;
      if (sett.morningWateringInterval != newVal) {
        sett.morningWateringInterval = newVal;
        needSave = true;
        Serial.printf("DEBUG: Updated morningWateringInterval to %u ms\n", sett.morningWateringInterval);
      }
    }
    if (doc.containsKey("morningWateringDuration")) {
      uint64_t tempVal = (uint64_t)doc["morningWateringDuration"];
      uint32_t newVal = (uint32_t)tempVal;
      if (sett.morningWateringDuration != newVal) {
        sett.morningWateringDuration = newVal;
        needSave = true;
        Serial.printf("DEBUG: Updated morningWateringDuration to %u ms\n", sett.morningWateringDuration);
      }
    }
    if (doc.containsKey("manualPumpOverride")) {
      bool val = doc["manualPumpOverride"];
      if (sett.manualPumpOverride != val) {
        sett.manualPumpOverride = val;
        needSave = true;
        Serial.printf("DEBUG: Updated manualPumpOverride to %s\n", sett.manualPumpOverride ? "true" : "false");
      }
    }
    if (doc.containsKey("previousManualPump")) {
      bool val = doc["previousManualPump"];
      if (sett.previousManualPump != val) {
        sett.previousManualPump = val;
        needSave = true;
        Serial.printf("DEBUG: Updated previousManualPump to %s\n", sett.previousManualPump ? "true" : "false");
      }
    }
    if (doc.containsKey("hydroMix")) {
      bool val = doc["hydroMix"];
      if (sett.hydroMix != val) {
        sett.hydroMix = val;
        needSave = true;
        Serial.printf("DEBUG: Updated hydroMix to %s\n", sett.hydroMix ? "true" : "false");
      }
    }
    if (doc.containsKey("hydroMixDuration")) {
      uint32_t val = doc["hydroMixDuration"];
      if (sett.hydroMixDuration != val) {
        sett.hydroMixDuration = val;
        needSave = true;
        Serial.printf("DEBUG: Updated hydroMixDuration to %u\n", sett.hydroMixDuration);
      }
    }
    if (doc.containsKey("hydroStart")) {
      uint32_t val = doc["hydroStart"];
      if (sett.hydroStart != val) {
        sett.hydroStart = val;
        needSave = true;
        Serial.printf("DEBUG: Updated hydroStart to %u\n", sett.hydroStart);
      }
    }
    if (doc.containsKey("fillState")) {
      bool val = doc["fillState"];
      if (sett.fillState != val) {
        sett.fillState = val;
        needSave = true;
        Serial.printf("Updated fillState to %s\n", sett.fillState ? "true" : "false");
      }
    }
    if (doc.containsKey("levelMin")) {
      float val = doc["levelMin"];
      if (sett.levelMin != val) {
        sett.levelMin = val;
        needSave = true;
        Serial.printf("Updated levelMin to %.1f\n", sett.levelMin);
      }
    }
    if (doc.containsKey("levelMax")) {
      float val = doc["levelMax"];
      if (sett.levelMax != val) {
        sett.levelMax = val;
        needSave = true;
        Serial.printf("Updated levelMax to %.1f\n", sett.levelMax);
      }
    }
    if (doc.containsKey("wateringMode")) {
      int val = doc["wateringMode"];
      if (sett.wateringMode != val) {
        if (val == 0 || val == 1) { // Явный переход в авто или ручной
          int prev = sett.wateringMode;
          if (prev == 2) { // Из forced
            resetForcedToNormal(val);
            needSave = true;
          } else {
            sett.wateringMode = val;
            needSave = true;
          }
          Serial.printf("DEBUG: Updated wateringMode to %d (explicit transition)\n", val);
        } else if (val == 2) {
          sett.wateringMode = 2; // Forced, но без активации
          needSave = true;
        }
      }
    }
    if (doc.containsKey("manualWateringInterval")) {
      uint64_t tempVal = (uint64_t)doc["manualWateringInterval"];
      uint32_t newVal = (uint32_t)tempVal;
      Serial.printf("[POST DEBUG] manualInterval JSON tempVal=%llu ms → newVal=%u ms (current sett=%u ms)\n", tempVal, newVal, sett.manualWateringInterval);
      if (sett.manualWateringInterval != newVal) {
        sett.manualWateringInterval = newVal;
        needSave = true;
        Serial.printf("DEBUG: Updated manualWateringInterval to %u ms\n", sett.manualWateringInterval);
      } else {
        Serial.println("[POST DEBUG] manualInterval no change, but forcing needSave for sync");
        needSave = true;
      }
    }
    if (doc.containsKey("manualWateringDuration")) {
      uint64_t tempVal = (uint64_t)doc["manualWateringDuration"];
      uint32_t newVal = (uint32_t)tempVal;
      Serial.printf("[POST DEBUG] manualDuration JSON tempVal=%llu ms → newVal=%u ms (current sett=%u ms)\n", tempVal, newVal, sett.manualWateringDuration);
      if (sett.manualWateringDuration != newVal) {
        sett.manualWateringDuration = newVal;
        needSave = true;
        Serial.printf("DEBUG: Updated manualWateringDuration to %u ms\n", sett.manualWateringDuration);
      } else {
        Serial.println("[POST DEBUG] manualDuration no change, but forcing needSave for sync");
        needSave = true;
      }
    }
    if (doc.containsKey("matMinHumidity")) {
      float val = doc["matMinHumidity"];
      if (sett.matMinHumidity != val) {
        sett.matMinHumidity = val;
        needSave = true;
        Serial.printf("DEBUG: Updated matMinHumidity to %.2f\n", sett.matMinHumidity);
      }
    }
    if (doc.containsKey("matMaxEC")) {
      float val = doc["matMaxEC"];
      if (sett.matMaxEC != val) {
        sett.matMaxEC = val;
        needSave = true;
        Serial.printf("DEBUG: Updated matMaxEC to %.2f\n", sett.matMaxEC);
      }
    }
    if (doc.containsKey("radCheckInterval")) {
      uint32_t val = doc["radCheckInterval"];
      if (sett.radCheckInterval != val) {
        sett.radCheckInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated radCheckInterval to %u ms\n", sett.radCheckInterval);
      }
    }
    if (doc.containsKey("maxWateringInterval")) {
      uint32_t val = doc["maxWateringInterval"];
      if (sett.maxWateringInterval != val) {
        sett.maxWateringInterval = val;
        needSave = true;
        Serial.printf("DEBUG: Updated maxWateringInterval to %u ms\n", sett.maxWateringInterval);
      }
    }
    if (doc.containsKey("forcedWateringDuration")) {
      uint32_t val = doc["forcedWateringDuration"];
      if (sett.forcedWateringDuration != val) {
        sett.forcedWateringDuration = val;
        needSave = true;
        Serial.printf("DEBUG: Updated forcedWateringDuration to %u ms\n", sett.forcedWateringDuration);
      }
    }
    if (doc.containsKey("wateringModes")) {
      JsonArray wmArray = doc["wateringModes"];
      if (wmArray.size() == 4) {
        bool modesChanged = false;
        for (int i = 0; i < 4; i++) {
          JsonObject wm = wmArray[i];
          if (wm.containsKey("enabled")) {
            bool val = wm["enabled"];
            if (sett.wateringModes[i].enabled != val) {
              sett.wateringModes[i].enabled = val;
              modesChanged = true;
            }
          }
          if (wm.containsKey("startHour")) {
            uint8_t val = wm["startHour"];
            if (sett.wateringModes[i].startHour != val) {
              sett.wateringModes[i].startHour = val;
              modesChanged = true;
            }
          }
          if (wm.containsKey("endHour")) {
            uint8_t val = wm["endHour"];
            if (sett.wateringModes[i].endHour != val) {
              sett.wateringModes[i].endHour = val;
              modesChanged = true;
            }
          }
          if (wm.containsKey("duration")) {
            uint32_t val = wm["duration"];
            if (sett.wateringModes[i].duration != val) {
              sett.wateringModes[i].duration = val;
              modesChanged = true;
            }
          }
        }
        if (modesChanged) {
          needSave = true;
        }
      }
    }
    if (needSave) {
      saveSettings(source);
    }
    publishSettings();
    publishAllSensors(); // Явная публикация после изменений
    return;
  } else if (strcmp(topic, "greenhouse/cmd/equipment") == 0) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
      Serial.println("Bad cmd/equipment JSON");
      return;
    }
    if (!doc.containsKey("equipment") || !doc.containsKey("state")) {
      Serial.println("Missing equipment/state in cmd/equipment");
      return;
    }
    String eq = doc["equipment"].as<String>();
    bool state = doc["state"].as<bool>();
    bool override = doc.containsKey("override") ? doc["override"].as<bool>() : false;
    bool needSave = false;
    if (eq == "fan") {
      sett.manualFan = true;
      sett.fanState = state;
      needSave = true;
      Serial.printf("Fan set %s\n", state ? "ON" : "OFF");
    } else if (eq == "heating") {
      sett.manualHeat = true;
      sett.heatState = state;
      needSave = true;
      Serial.printf("Heating set %s\n", state ? "ON" : "OFF");
    } else if (eq == "watering") {
      sett.manualPump = true;
      sett.manualPumpOverride = override;
      sett.pumpState = state;
      needSave = true;
      Serial.printf("Watering set %s, override=%d\n", state ? "ON" : "OFF", override);
    } else if (eq == "solution-heating") {
      sett.manualSol = true;
      sett.solHeatState = state;
      needSave = true;
      Serial.printf("Solution heating set %s\n", state ? "ON" : "OFF");
    } else if (eq == "fog-mode-auto") {
      sett.fogMode = 0;
      sett.forceFogOn = false;
      sett.fogState = false;
      needSave = true;
      Serial.println("Fog mode set AUTO");
    } else if (eq == "fog-mode-manual") {
      sett.fogMode = 1;
      sett.forceFogOn = false;
      sett.fogState = false;
      needSave = true;
      Serial.println("Fog mode set MANUAL");
    } else if (eq == "fog-mode-forced") {
      sett.fogMode = 2;
      sett.forceFogOn = state;
      needSave = true;
      Serial.println(state ? "Fog forced ON" : "Fog forced OFF");
    } else {
      Serial.printf("Unknown equipment: %s\n", eq.c_str());
    }
    if (needSave) {
      saveSettings("MQTT_cmd_equipment");
      publishSettings();
      publishAllSensors(); // Явная публикация после изменений
    }
    return;
  } else if (strcmp(topic, "greenhouse/cmd/window") == 0) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
      Serial.println("Bad cmd/window JSON");
      return;
    }
    if (!doc.containsKey("direction")) {
      Serial.println("Missing direction in cmd/window");
      return;
    }
    String dir = doc["direction"].as<String>();
    uint32_t dur = doc.containsKey("duration") ? doc["duration"].as<uint32_t>() : 0;
    if (dir == "up" || dir == "down") {
      sett.manualWindow = true;
      sett.windowCommand = dir;
      sett.windowDuration = dur;
      saveSettings("MQTT_window_cmd");
    } else if (dir == "stop") {
      sett.windowCommand = "stop";
    } else {
      Serial.println("Invalid direction");
    }
    return;
  } else if (strcmp(topic, "greenhouse/cmd/request_settings") == 0) {
    publishSettings();
    return;
  } else if (strcmp(topic, "greenhouse/cmd/request_history") == 0) {
    publishHistory();
    return;
  } else if (strcmp(topic, "greenhouse/cmd/hydro") == 0) {
    float level;
    xQueuePeek(qLevel, &level, 0);
    DateTime nowDt = rtc.now();
    uint32_t nowUnix = nowDt.unixtime();
    if (sett.hydroMix) {
      sett.hydroMix = false;
      sett.forceWateringActive = false;
      sett.forceWateringOverride = false;
      sett.wateringMode = sett.prevWateringMode;
      sett.pumpState = false;
      sett.manualPump = sett.previousManualPump;
      sett.hydroStartUnix = 0;
      Serial.println("Hydro mix OFF");
    } else {
      if (sett.morningWateringActive) {
        sett.pendingMorningComplete = true;
        LOG("DEBUG MORNING-WATER: Hydro start during morning — pending complete set\n");
      }
      sett.previousManualPump = sett.manualPump;
      sett.prevWateringMode = sett.wateringMode;
      sett.hydroMix = true;
      sett.hydroStartUnix = nowUnix;
      sett.pumpState = true;
      sett.forceWateringActive = true;
      uint32_t duration = sett.hydroMixDuration;
      if (duration == 0) duration = 10 * 60 * 1000UL;
      sett.forceWateringEndTimeUnix = nowUnix + (duration / 1000UL);
      sett.wateringMode = 2;
      if (sett.forceWateringOverride) sett.forceWateringOverride = false;
      Serial.printf("Hydro mix ON for %u ms (unix %lu)\n", duration, sett.hydroStartUnix);
    }
    saveSettings("cmd_hydro");
    publishSettings();
    publishAllSensors(); // Явная публикация после hydro
    return;
  }
  Serial.printf("Unknown command topic: %s\n", topic);
}
void startWindowMovement(String direction, uint32_t duration) {
  windowMoving = true;
  windowDirection = direction;
  windowStartTime = millis();
  moveDuration = duration;
  if (direction == "up") {
    digitalWrite(REL_WINDOW_UP, HIGH);
    digitalWrite(REL_WINDOW_DN, LOW);
    Serial.printf("Starting window movement UP for %u ms\n", duration);
  } else if (direction == "down") {
    digitalWrite(REL_WINDOW_UP, LOW);
    digitalWrite(REL_WINDOW_DN, HIGH);
    Serial.printf("Starting window movement DOWN for %u ms\n", duration);
  }
}
void stopWindowMovement() {
  digitalWrite(REL_WINDOW_UP, LOW);
  digitalWrite(REL_WINDOW_DN, LOW);
  windowMoving = false;
  moveDuration = 0;
}
void updateWindowPosition() {
  if (!windowMoving) return;
  uint32_t elapsed = millis() - windowStartTime;
  float percentage = 0;
  uint32_t fullTime = (windowDirection == "up") ? sett.openTime : sett.closeTime;
  percentage = (float)elapsed / fullTime * 100.0;
  if (windowDirection == "up") {
    windowPos = constrain(windowPos + percentage, 0, 100);
  } else if (windowDirection == "down") {
    windowPos = constrain(windowPos - percentage, 0, 100);
  }
  stopWindowMovement();
  Serial.printf("Window stopped. Elapsed: %u ms, Percentage change: %.2f%%, New position: %d%%\n", elapsed, percentage, windowPos);
  if (client.connected()) {
    client.publish("greenhouse/esp32/window", String(windowPos).c_str(), true);
  }
}
bool forceUpdateAndValidateTime() {
  Serial.println("Принудительное обновление времени через NTP для синхронизации RTC...");
  bool updateSuccess = timeClient.forceUpdate();
  if (updateSuccess) {
    unsigned long epochTime = timeClient.getEpochTime();
    if (epochTime > 1704067200UL) {
      rtc.adjust(DateTime(epochTime));
      Serial.println("RTC синхронизировано с NTP и время выглядит корректно.");
      Serial.print("Текущее время (UTC): ");
      Serial.print(timeClient.getFormattedTime());
      Serial.print(" (Epoch: ");
      Serial.print(epochTime);
      Serial.println(")");
      return true;
    } else {
      Serial.print("Время обновлено, но кажется некорректным (Epoch: ");
      Serial.print(epochTime);
      Serial.println("). RTC не синхронизировано.");
      return false;
    }
  } else {
    Serial.println("Не удалось обновить время через NTP. RTC остается без изменений.");
    return false;
  }
}
void networkTimeManagementTask(void *parameter) {
  const char *ssid = WIFI_SSID;
  const char *password = WIFI_PASS;
  static unsigned long lastStatusPublish = 0;
  const unsigned long statusPublishInterval = 4 * 60 * 1000;
  static unsigned long lastDebugPrint = 0;
  const unsigned long debugPrintInterval = 60 * 1000;
  const unsigned long CRITICAL_UPDATE_INTERVAL = 5 * 60 * 1000;
  Serial.println("Network & Time Management Task started");
  while (true) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastDebugPrint >= debugPrintInterval) {
      Serial.print("NetworkTask: Running for ");
      Serial.print(currentMillis / 1000);
      Serial.println(" seconds");
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("NetworkTask: Wi-Fi connected. Current RTC time: ");
        DateTime now = rtc.now();
        if (now.unixtime() > 1704067200UL) {
          Serial.printf("%02d:%02d:%02d %02d/%02d/%04d\n", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
        } else {
          Serial.println("NOT SET");
        }
      } else {
        Serial.println("NetworkTask: Wi-Fi NOT connected");
      }
      lastDebugPrint = currentMillis;
    }
    if ((currentMillis - lastNetworkCheckTime > NETWORK_CHECK_INTERVAL) || forceWifiReconnect || (WiFi.status() != WL_CONNECTED)) {
      if (WiFi.status() != WL_CONNECTED || forceWifiReconnect) {
        Serial.println("Wi-Fi disconnected or reconnect forced. Attempting to reconnect...");
        WiFi.disconnect();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        WiFi.begin(ssid, password);
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < 20000)) {
          Serial.print(".");
          vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        if (WiFi.status() == WL_CONNECTED) {
          wifiConnectedOnce = true;
          Serial.println("\nWi-Fi reconnected successfully!");
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
          lastStatusPublish = 0;
        } else {
          wifiConnectedOnce = false;
          Serial.println("\nFailed to reconnect to Wi-Fi");
        }
      }
      lastNetworkCheckTime = currentMillis;
      forceWifiReconnect = false;
    }
    if (wifiConnectedOnce) {
      DateTime now = rtc.now();
      bool criticalNeed = (now.unixtime() <= 1704067200UL);
      unsigned long updateInterval = criticalNeed ? CRITICAL_UPDATE_INTERVAL : TIME_UPDATE_INTERVAL;
      bool needUpdateTime = (now.unixtime() <= 1704067200UL) || (currentMillis - lastTimeUpdateTime > updateInterval);
      if (needUpdateTime) {
        if (now.unixtime() <= 1704067200UL) {
          Serial.println("RTC time not set — attempting NTP sync...");
        } else {
          Serial.println("Scheduled NTP time update for RTC sync...");
        }
        bool success = false;
        for (int retry = 0; retry < 3 && !success; retry++) {
          if (timeClient.forceUpdate()) {
            unsigned long epoch = timeClient.getEpochTime();
            if (epoch > 1704067200UL) {
              success = true;
              Serial.printf("NTP retry %d OK: epoch %lu (%s)\n", retry + 1, epoch, timeClient.getFormattedTime().c_str());
            } else {
              Serial.printf("NTP retry %d: Invalid epoch %lu\n", retry + 1, epoch);
            }
          } else {
            Serial.printf("NTP retry %d failed\n", retry + 1);
          }
          if (!success && retry < 2) {
            vTaskDelay(2000 / portTICK_PERIOD_MS);
          }
        }
        if (success) {
          unsigned long epoch = timeClient.getEpochTime();
          rtc.adjust(DateTime(epoch));
          Serial.println("✅ NTP time sync successful and RTC updated.");
        } else {
          Serial.println("⚠️ NTP sync failed after retries — will retry soon. RTC remains unchanged.");
        }
        lastTimeUpdateTime = currentMillis;
      }
    }
    if (wifiConnectedOnce && (currentMillis - lastStatusPublish >= statusPublishInterval)) {
      if (client.connected()) {
        if (client.publish("greenhouse/esp32/status", "online", true)) {
          Serial.println("Periodic 'online' status published via MQTT");
        } else {
          Serial.println("Failed to publish periodic 'online' status");
        }
      } else {
        Serial.println("MQTT not connected, skipping periodic 'online' status publish");
      }
      lastStatusPublish = currentMillis;
    }
    vTaskDelay(30000 / portTICK_PERIOD_MS);
  }
}
uint16_t readU16ModMaster(uint8_t id, uint16_t reg) {
  mb.begin(id, RS485);
  uint8_t result = mb.readHoldingRegisters(reg, 1);
  uint32_t t0 = millis();
  const uint32_t timeout = 1000;
  while (mb.getResponseBuffer(0) == 0 && result == mb.ku8MBResponseTimedOut && millis() - t0 < timeout) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  if (result == mb.ku8MBSuccess) {
    uint16_t value = mb.getResponseBuffer(0);
    LOG("Modbus read success for ID %d, reg %d: %u\n", id, reg, value);
    return value;
  }
  LOG("ERROR: Modbus read failed for ID %d, reg %d, code: 0x%02X\n", id, reg, result);
  return 65535;
}
float readFloatModMaster(uint8_t id, uint16_t reg) {
  mb.begin(id, RS485);
  uint8_t result = mb.readHoldingRegisters(reg, 2);
  uint32_t t0 = millis();
  const uint32_t timeout = 1000;
  while (mb.getResponseBuffer(0) == 0 && result == mb.ku8MBResponseTimedOut && millis() - t0 < timeout) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  if (result == mb.ku8MBSuccess) {
    uint32_t raw = ((uint32_t)mb.getResponseBuffer(1) << 16) | mb.getResponseBuffer(0);
    float value = *(float *)&raw;
    if (value < -40 || value > 1000) {
      LOG("ERROR: Invalid value for ID %d, reg %d: %.2f\n", id, reg, value);
      return NAN;
    }
    LOG("Modbus read success for ID %d, reg %d: %.2f\n", id, reg, value);
    return value;
  }
  LOG("ERROR: Modbus read failed for ID %d, reg %d, code: 0x%02X\n", id, reg, result);
  return NAN;
}
void modbusTask(void *parameter) {
  for (;;) {
    LOG("--- Начало чтения датчиков T/H (ID %d & %d) ---\n", SLAVE_TEMP1, SLAVE_TEMP2);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint16_t raw_h1 = readU16ModMaster(SLAVE_TEMP1, 0);
    float h1 = (raw_h1 == 65535 || raw_h1 == 0 || raw_h1 > 1100) ? NAN : raw_h1 / 10.0f;
    LOG("H1 raw=%u → h1=%.1f%% (valid: %s)\n", raw_h1, h1, isnan(h1) ? "false" : "true");
    uint16_t raw_t1 = readU16ModMaster(SLAVE_TEMP1, 1);
    float t1 = (raw_t1 == 65535 || raw_t1 == 0 || raw_t1 < -100 || raw_t1 > 600) ? NAN : raw_t1 / 10.0f;
    LOG("T1 raw=%u → t1=%.1f°C (valid: %s)\n", raw_t1, t1, isnan(t1) ? "false" : "true");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint16_t raw_h2 = readU16ModMaster(SLAVE_TEMP2, 0);
    float h2 = (raw_h2 == 65535 || raw_h2 == 0 || raw_h2 > 1100) ? NAN : raw_h2 / 10.0f;
    LOG("H2 raw=%u → h2=%.1f%% (valid: %s)\n", raw_h2, h2, isnan(h2) ? "false" : "true");
    uint16_t raw_t2 = readU16ModMaster(SLAVE_TEMP2, 1);
    float t2 = (raw_t2 == 65535 || raw_t2 == 0 || raw_t2 < -100 || raw_t2 > 600) ? NAN : raw_t2 / 10.0f;
    LOG("T2 raw=%u → t2=%.1f°C (valid: %s)\n", raw_t2, t2, isnan(t2) ? "false" : "true");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    float avgHum = (isnan(h1) || isnan(h2)) ? (isnan(h1) ? (isnan(h2) ? NAN : h2) : h1) : (h1 + h2) / 2.0f;
    float avgTemp = (isnan(t1) || isnan(t2)) ? (isnan(t1) ? (isnan(t2) ? NAN : t2) : t1) : (t1 + t2) / 2.0f;
    LOG("Усреднённые: H=%.1f%% (h1=%.1f, h2=%.1f), T=%.1f°C (t1=%.1f, t2=%.1f)\n", avgHum, h1, h2, avgTemp, t1, t2);
    LOG("--- Начало чтения других датчиков (uint16_t) ---\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint16_t raw_sol_u16 = readU16ModMaster(SLAVE_SOL, 0);
    float sol = raw_sol_u16 / 10.0;
    LOG("Датчик раствора (ID %d): T=%.1f°C (сырое: %u)\n", SLAVE_SOL, sol, raw_sol_u16);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint16_t rawLevel_u16 = readU16ModMaster(SLAVE_LEVEL, 4);
    float level_calibrated = mapFloat(rawLevel_u16, sett.levelMin, sett.levelMax, 0, 100);
    level_calibrated = constrain(level_calibrated, 0, 100);
    LOG("Датчик уровня (ID %d): Level_Cal=%.1f%% (сырое: %u, Min: %.1f, Max: %.1f)\n", SLAVE_LEVEL, level_calibrated, rawLevel_u16, sett.levelMin, sett.levelMax);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint16_t raw_wind_u16 = readU16ModMaster(SLAVE_WIND, 0);
    float wind = raw_wind_u16 / 10.0;
    LOG("Датчик ветра (ID %d): Wind=%.1f m/s (сырое: %u)\n", SLAVE_WIND, wind, raw_wind_u16);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    LOG("--- Начало чтения НОВЫХ датчиков (float) ---\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint16_t raw_pyrano = readU16ModMaster(SLAVE_PYRANO, 0);
    float pyrano = raw_pyrano;
    LOG("Пиранометр (ID %d): Pyrano=%.2f W/m2\n", SLAVE_PYRANO, pyrano);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint16_t raw_hum = readU16ModMaster(SLAVE_MAT_SENSOR, 0);
    uint16_t raw_temp = readU16ModMaster(SLAVE_MAT_SENSOR, 1);
    uint16_t raw_ec = readU16ModMaster(SLAVE_MAT_SENSOR, 2);
    uint16_t raw_ph = readU16ModMaster(SLAVE_MAT_SENSOR, 3);
    float hum = raw_hum / 10.0;
    float temp = raw_temp / 10.0;
    float ec = raw_ec / 100.0;
    float ph = raw_ph / 10.0;
    MatSensorData matData;
    matData.temperature = temp;
    matData.humidity = hum;
    matData.ec = ec;
    matData.ph = ph;
    LOG("Датчик мата (ID %d): H=%.1f%%, T=%.1f°C, EC=%u µS/cm, pH=%.2f\n",
        SLAVE_MAT_SENSOR, matData.humidity, matData.temperature, (uint16_t)matData.ec, matData.ph);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    LOG("- Начало чтения уличного датчика T/H (ID %d) -", SLAVE_OUTDOOR);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint16_t raw_outdoor_h = readU16ModMaster(SLAVE_OUTDOOR, 0);
    float outdoorHum = raw_outdoor_h / 10.0;
    uint16_t raw_outdoor_t = readU16ModMaster(SLAVE_OUTDOOR, 1);
    float outdoorTemp = raw_outdoor_t / 10.0;
    LOG("Датчик улицы (ID %d): H=%.1f%%, T=%.1f°C (сырое H: %u, T: %u)", SLAVE_OUTDOOR, outdoorHum, outdoorTemp, raw_outdoor_h, raw_outdoor_t);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    SensorData avgData = { avgTemp, avgHum };
    xQueueOverwrite(qAvg, &avgData);
    xQueueOverwrite(qSol, &sol);
    xQueueOverwrite(qLevel, &level_calibrated);
    xQueueOverwrite(qWind, &wind);
    xQueueOverwrite(qPyrano, &pyrano);
    xQueueOverwrite(qMat, &matData);
    SensorData outdoorData = { outdoorTemp, outdoorHum };
    xQueueOverwrite(qOutdoor, &outdoorData);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}
void logicTask(void *parameter) {
  float avgTemp, avgHum, sol, level, wind, outdoorTemp, outdoorHum;
  float pyrano;
  MatSensorData mat;
  static bool heat = false;
  static bool fan = false;
  static bool fog = false;
  static bool pump = false;
  static bool solHeat = false;
  static uint32_t windLockStart = 0;
  static bool windRestricted = false;
  static bool prevWindRestricted = false;
  static uint32_t lastHistory = 0;
  static uint32_t lastHour = 0;
  static uint32_t lastWater = 0;
  static uint32_t taskFogStartTime = 0;
  static uint32_t taskLastFog = 0;
  static bool fogValveOn = false;
  static uint32_t fogValveStartTime = 0;
  static uint32_t lastFanChange = 0;
  static uint32_t lastHeatChange = 0;
  static uint32_t lastSolHeatChange = 0;
  const uint32_t minStateChangeInterval = 5000;
  const uint32_t minWindowStateChangeInterval = 30000;
  static uint32_t lastWindowMoveCommand = 0;
  static bool sensorsValid = false;
  static uint32_t lastManualHeatCheck = 0, lastManualFanCheck = 0;
  static uint32_t lastManualSolCheck = 0;
  static uint32_t lastManualPumpCheck = 0;
  static int lastDay = -1;
  static uint32_t lastRadCheck = 0;
  static float lastAvgTemp = NAN;
  static float lastAvgHum = NAN;
  const float tempHysteresis = 0.2;
  const float humHysteresis = 2.0;
  static unsigned long lastMqttCheck = 0;
  static bool initialized = false;
  static bool forcedPumpStarted = false; // Флаг для отслеживания старта pump в forced
  for (;;) {
    unsigned long now = millis();
    SensorData avg;
    if (xQueuePeek(qAvg, &avg, 0) == pdTRUE) {
      avgTemp = avg.temperature;
      avgHum = avg.humidity;
    } else {
      avgTemp = NAN;
      avgHum = NAN;
    }
    xQueuePeek(qSol, &sol, 0);
    xQueuePeek(qLevel, &level, 0);
    xQueuePeek(qWind, &wind, 0);
    SensorData outdoor;
    xQueuePeek(qOutdoor, &outdoor, 0);
    outdoorTemp = outdoor.temperature;
    outdoorHum = outdoor.humidity;
    xQueuePeek(qPyrano, &pyrano, 0);
    xQueuePeek(qMat, &mat, 0);
    sensorsValid = !isnan(avgTemp) && avgTemp != 0;
    bool needSave = false;
    const char *saveCaller = "unknown";
    if (!initialized) {
      forcedPumpStarted = false; // Сброс флага при init
      if (sett.fogState) {
        digitalWrite(REL_FOG_PUMP, LOW);
        digitalWrite(REL_FOG_VALVE, LOW);
        fog = false;
        fogValveOn = false;
        sett.fogState = false;
        needSave = true;
        saveCaller = "fog_init";
      }
      taskLastFog = 0;
      taskFogStartTime = 0;
      fogValveStartTime = 0;
      DateTime nowDt = rtc.now();
      uint32_t nowUnix = nowDt.unixtime();
      int currentDay = nowDt.day();
      int h = nowDt.hour(), m = nowDt.minute();
      int currentMin = h * 60 + m;
      int startMin = sett.wateringStartHour * 60 + sett.wateringStartMinute;
      int endMin = sett.wateringEndHour * 60 + sett.wateringEndMinute;
      if (currentDay != lastDay) {
        sett.cycleCount = 0;
        // НЕ сбрасываем radSum при новом дне для morning
        sett.morningStartedToday = false;
        sett.morningWateringActive = false;
        sett.currentMorningWatering = 0;
        sett.lastMorningWateringEndUnix = 0;
        lastDay = currentDay;
        LOG("DEBUG AUTO-WATER: New day started. Reset cycleCount=%d, morningStartedToday=false\n", sett.cycleCount);
        needSave = true;
        saveCaller = "new_day_reset";
      }
      if (sett.morningWateringActive && sett.currentMorningWatering > 0 && sett.morningWateringCount > 0) {
        uint32_t dayStartEstimate = nowUnix - (nowUnix % 86400);
        if (sett.lastMorningWateringStartUnix >= (dayStartEstimate - 86400)) {
          uint32_t elapsedMorning = nowUnix - sett.lastMorningWateringStartUnix;
          uint32_t durationSec = sett.morningWateringDuration / 1000UL;
          if (elapsedMorning < durationSec) {
            sett.pumpState = true;
            LOG("DEBUG MORNING-RESTORE: Restored morning #%d (%.1f sec left)\n", sett.currentMorningWatering, durationSec - elapsedMorning);
          } else if (elapsedMorning >= (sett.morningWateringInterval / 1000UL)) {
            if (sett.currentMorningWatering < sett.morningWateringCount) {
              sett.currentMorningWatering++;
              sett.lastMorningWateringStartUnix = nowUnix;
              sett.pumpState = true;
              LOG("DEBUG MORNING-RESTORE: Started next morning #%d after restart\n", sett.currentMorningWatering);
            } else {
              sett.morningWateringActive = false;
              // НЕ сбрасываем radSum; lastWateringStartUnix на lastMorningStart
              sett.lastWateringStartUnix = sett.lastMorningWateringStartUnix;
              LOG("DEBUG MORNING-RESTORE: Sequence completed, morning inactive, lastWateringStart=lastMorningStart\n");
            }
          } else {
            sett.pumpState = false;
            LOG("DEBUG MORNING-RESTORE: Morning interval not elapsed yet (%.1f sec left)\n", (sett.morningWateringInterval / 1000UL) - elapsedMorning);
          }
        } else {
          sett.morningWateringActive = false;
          sett.morningStartedToday = false;
          sett.currentMorningWatering = 0;
          LOG("DEBUG MORNING-RESTORE: Old day detected - reset morning state\n");
        }
        needSave = true;
        saveCaller = "morning_restore";
      }
      if (sett.morningWateringActive && currentMin >= endMin) {
        sett.morningWateringActive = false;
        sett.morningStartedToday = true;
        sett.currentMorningWatering = sett.morningWateringCount;
        sett.lastWateringStartUnix = sett.lastMorningWateringStartUnix;
        LOG("DEBUG MORNING-RESTORE: Outside end time - cancel morning sequence, lastWateringStart=lastMorningStart\n");
        needSave = true;
        saveCaller = "morning_restore_endtime";
      }
      if ((sett.wateringMode == 0 || sett.wateringMode == 1) && currentMin >= startMin && currentMin < endMin) {
        uint32_t elapsedWater = nowUnix - sett.lastWateringStartUnix;
        uint32_t durationSec = (sett.wateringMode == 0 ? sett.pumpTime : sett.manualWateringDuration) / 1000UL;
        if (sett.pumpState && elapsedWater < durationSec) {
          LOG("DEBUG WATER-RESTORE: Restored %s watering (%.1f sec left)\n", (sett.wateringMode == 0 ? "auto" : "manual"), durationSec - elapsedWater);
        } else {
          sett.pumpState = false;
        }
      }
      if (sett.forceWateringActive) {
        if (nowUnix >= sett.forceWateringEndTimeUnix) {
          sett.forceWateringActive = false;
          sett.pumpState = false;
          sett.fillState = false;
          sett.hydroMix = false;
          sett.wateringMode = sett.prevWateringMode;
          sett.hydroStartUnix = 0;
          sett.forceWateringEndTimeUnix = 0;
          // Если был реальный полив, сбросить таймеры здесь
          if (sett.forcedWateringPerformed) {
            sett.lastWateringStartUnix = nowUnix;
            int targetMode = sett.prevWateringMode;
            if (targetMode == 0) {
              sett.radSum = 0.0f;
              sett.cycleCount++;
            } else if (targetMode == 1) {
              sett.lastManualWateringTimeUnix = nowUnix;
            }
            sett.forcedWateringPerformed = false;
            LOG("DEBUG FORCED-RESTORE: Forced timeout after restart - ended, timers reset (performed)\n");
          } else {
            LOG("DEBUG FORCED-RESTORE: Forced timeout after restart - ended, no reset (no pump)\n");
          }
          needSave = true;
          saveCaller = "forced_timeout_restore";
        } else {
          bool shouldPump = !(sett.fillState && !sett.hydroMix);
          if (shouldPump) {
            sett.pumpState = true;
            forcedPumpStarted = true;
            sett.forcedWateringPerformed = true;
          } else {
            sett.pumpState = false;
          }
          if (sett.hydroMix) {
            uint32_t elapsedHydro = nowUnix - sett.hydroStartUnix;
            uint32_t hydroSec = sett.hydroMixDuration / 1000UL;
            if (elapsedHydro >= hydroSec) {
              sett.hydroMix = false;
              sett.hydroStartUnix = 0;
              LOG("DEBUG HYDRO-RESTORE: Hydro timeout after restart - ended\n");
              needSave = true;
            }
          }
          if (sett.fillState && level >= 99.0) {
            sett.fillState = false;
            LOG("DEBUG FILL-RESTORE: Fill complete after restart - ended\n");
            needSave = true;
          }
          LOG("DEBUG FORCED-RESTORE: Restored forced (%.1f sec left)\n", sett.forceWateringEndTimeUnix - nowUnix);
          needSave = true;
          saveCaller = "forced_restore";
        }
      } else if (sett.forceWateringOverride) {
        sett.pumpState = false;
        LOG("DEBUG OVERRIDE-RESTORE: Override active after restart - pump OFF\n");
      }
      initialized = true;
      Serial.println("LogicTask initialized with watering state restored");
    }
    DateTime nowTime = rtc.now();
    uint32_t nowUnix = nowTime.unixtime();
    int h = nowTime.hour();
    int m = nowTime.minute();
    int currentMin = h * 60 + m;
    int startMin = sett.wateringStartHour * 60 + sett.wateringStartMinute;
    int endMin = sett.wateringEndHour * 60 + sett.wateringEndMinute;
    int currentDay = nowTime.day();
    if (currentDay != lastDay) {
      sett.cycleCount = 0;
      // НЕ сбрасываем radSum при новом дне
      sett.morningStartedToday = false;
      sett.morningWateringActive = false;
      sett.currentMorningWatering = 0;
      sett.lastMorningWateringEndUnix = 0;
      lastDay = currentDay;
      LOG("DEBUG AUTO-WATER: New day started. Reset cycleCount=%d, morningStartedToday=false\n", sett.cycleCount);
      needSave = true;
      saveCaller = "new_day_reset";
    }
    if ((sett.wateringMode == 0 || sett.wateringMode == 1) && sett.morningWateringCount > 0 && !sett.forceWateringActive && !sett.forceWateringOverride) {
      uint32_t morningWindowMin = (sett.morningWateringCount > 0) ? (sett.morningWateringCount - 1UL) * (sett.morningWateringInterval / 60000UL) : 0;
      if (!sett.morningStartedToday && currentMin >= startMin && currentMin < endMin && currentMin < (startMin + morningWindowMin)) {
        sett.morningStartedToday = true;
        sett.morningWateringActive = true;
        sett.currentMorningWatering = 1;
        sett.lastMorningWateringStartUnix = nowUnix;
        sett.pumpState = true;
        sett.lastWateringStartUnix = nowUnix; // Для morning используем текущий
        // НЕ сбрасываем radSum
        sett.lastManualWateringTimeUnix = nowUnix;
        sett.lastSunTimeUnix = nowUnix;
        sett.cycleCount++;
        needSave = true;
        saveCaller = "morning_watering_started";
        LOG("DEBUG MORNING-WATER: Started sequence! Count=%u, Interval=%u ms, Duration=%u ms, Cycle %d (unix %lu)\n", sett.morningWateringCount, sett.morningWateringInterval, sett.morningWateringDuration, sett.cycleCount, nowUnix);
      }
      if (sett.morningWateringActive) {
        uint32_t elapsedMorning = nowUnix - sett.lastMorningWateringStartUnix;
        uint32_t durationSec = sett.morningWateringDuration / 1000UL;
        if (sett.pumpState && elapsedMorning >= durationSec) {
          sett.pumpState = false;
          sett.lastMorningWateringEndUnix = nowUnix;
          LOG("DEBUG MORNING-WATER: Ended watering #%u after %u sec (unix %lu)\n", sett.currentMorningWatering, elapsedMorning, nowUnix);
          if (sett.currentMorningWatering >= sett.morningWateringCount) {
            sett.morningWateringActive = false;
            // НЕ сбрасываем radSum; lastWateringStartUnix уже на старте последнего
            if (sett.wateringMode == 1) sett.lastManualWateringTimeUnix = sett.lastMorningWateringStartUnix;
            needSave = true;
            saveCaller = "morning_watering_completed";
            LOG("DEBUG MORNING-WATER: Sequence completed! lastWateringStart remains lastMorningStart (unix %lu)\n", sett.lastWateringStartUnix);
            // Проверка на немедленный запуск auto после morning, если radSum >= threshold
            if (sett.wateringMode == 0 && sett.radSum >= (sett.radThreshold * 1e6)) {
              uint32_t minSec = sett.minWateringInterval / 1000UL;
              uint32_t elapsedFromLastMorning = nowUnix - sett.lastMorningWateringStartUnix;
              if (elapsedFromLastMorning >= minSec) {
                sett.lastWateringStartUnix = nowUnix;
                sett.cycleCount++;
                sett.radSum = 0.0f;
                sett.lastSunTimeUnix = nowUnix;
                sett.pumpState = true;
                needSave = true;
                saveCaller = "auto_after_morning_rad";
                LOG("DEBUG AUTO-WATER: STARTED after morning by radiation! Cycle %d (unix %lu)\n", sett.cycleCount, nowUnix);
              }
            }
          }
        }
        uint32_t intervalSec = sett.morningWateringInterval / 1000UL;
        if (!sett.pumpState && sett.currentMorningWatering < sett.morningWateringCount && (nowUnix - sett.lastMorningWateringStartUnix) >= intervalSec && currentMin < endMin) {
          sett.currentMorningWatering++;
          sett.lastMorningWateringStartUnix = nowUnix;
          sett.pumpState = true;
          sett.lastWateringStartUnix = nowUnix;
          // НЕ сбрасываем radSum
          sett.lastManualWateringTimeUnix = nowUnix;
          sett.lastSunTimeUnix = nowUnix;
          sett.cycleCount++;
          needSave = true;
          saveCaller = "morning_watering_next";
          LOG("DEBUG MORNING-WATER: Started next watering #%u, Cycle %d (unix %lu)\n", sett.currentMorningWatering, sett.cycleCount, nowUnix);
        }
      }
    }
    if (sett.wateringMode == 0 && !sett.forceWateringActive && !sett.forceWateringOverride && !sett.morningWateringActive) {
      static uint32_t lastAutoWateringCheck = 0;
      if (millis() - lastAutoWateringCheck >= sett.radCheckInterval) {
        lastAutoWateringCheck = millis();
        LOG("DEBUG AUTO-WATER: Auto watering check interval elapsed. Current radSum=%.2f J/m2, threshold=%.2f MJ/m2 (%.2f J/m2)\n", sett.radSum, sett.radThreshold, sett.radThreshold * 1e6);
        float pyrano;
        xQueuePeek(qPyrano, &pyrano, 0);
        if (!isnan(pyrano) && pyrano > 0) {
          float deltaRad = pyrano * (sett.radCheckInterval / 1000.0);
          sett.radSum += deltaRad;
          sett.lastSunTimeUnix = nowUnix;
          LOG("DEBUG AUTO-WATER: Radiation accumulation: pyrano=%.2f W/m2, delta=%.2f J/m2, new radSum=%.2f J/m2 (unix %lu)\n", pyrano, deltaRad, sett.radSum, nowUnix);
        } else {
          LOG("DEBUG AUTO-WATER: No radiation detected (pyrano=%.2f), checking reset\n", pyrano);
          uint32_t resetSec = sett.radSumResetInterval / 1000UL;
          if ((nowUnix - sett.lastSunTimeUnix) >= resetSec) {
            sett.radSum = 0.0f;
            sett.lastSunTimeUnix = nowUnix;
            LOG("DEBUG AUTO-WATER: radSum reset to 0 due to no sun for %u sec (unix %lu)\n", resetSec, nowUnix);
          }
        }
        if (currentMin >= startMin && currentMin < endMin) {
          LOG("DEBUG AUTO-WATER: Within watering hours (%02d:%02d - %02d:%02d). Checking conditions...\n", sett.wateringStartHour, sett.wateringStartMinute, sett.wateringEndHour, sett.wateringEndMinute);
          float level;
          xQueuePeek(qLevel, &level, 0);
          if (level >= 2) {
            LOG("DEBUG AUTO-WATER: Level OK (%.1f%% >=10). Cycles left: %d/%d\n", level, sett.maxWateringCycles - sett.cycleCount, sett.maxWateringCycles);
            if (sett.cycleCount < sett.maxWateringCycles) {
              uint32_t minSec = sett.minWateringInterval / 1000UL;
              uint32_t elapsedWater = nowUnix - sett.lastWateringStartUnix;
              if (elapsedWater >= minSec) {
                LOG("DEBUG AUTO-WATER: Min interval OK (last watering %u sec ago >= %u sec)\n", elapsedWater, minSec);
                MatSensorData mat;
                xQueuePeek(qMat, &mat, 0);
                if (!isnan(mat.humidity) && mat.humidity > sett.matMinHumidity) {
                  LOG("DEBUG AUTO-WATER: Mat humidity OK (%.1f%% > %.1f%%)\n", mat.humidity, sett.matMinHumidity);
                  if (!isnan(mat.ec) && mat.ec < sett.matMaxEC) {
                    LOG("DEBUG AUTO-WATER: Mat EC OK (%.2f < %.2f dS/m)\n", mat.ec, sett.matMaxEC);
                    bool radCondition = (sett.radSum >= (sett.radThreshold * 1e6));
                    uint32_t maxSec = sett.maxWateringInterval / 1000UL;
                    bool maxIntervalCondition = (elapsedWater >= maxSec);
                    if (radCondition || maxIntervalCondition) {
                      sett.lastWateringStartUnix = nowUnix;
                      sett.lastManualWateringTimeUnix = nowUnix;
                      sett.cycleCount++;
                      sett.radSum = 0.0f;
                      sett.lastSunTimeUnix = nowUnix;
                      sett.pumpState = true;
                      needSave = true;
                      saveCaller = (radCondition ? "auto_watering_started" : "auto_watering_by_max_interval");
                      LOG("DEBUG AUTO-WATER: STARTED by %s! Cycle %d, radSum reset (unix %lu)\n", (radCondition ? "radiation" : "max interval"), sett.cycleCount, nowUnix);
                    } else if (mat.humidity < (sett.matMinHumidity - 10.0)) {
                      sett.lastWateringStartUnix = nowUnix;
                      sett.lastManualWateringTimeUnix = nowUnix;
                      sett.cycleCount++;
                      sett.radSum = 0.0f;
                      sett.lastSunTimeUnix = nowUnix;
                      sett.pumpState = true;
                      needSave = true;
                      saveCaller = "extra_watering_started";
                      LOG("DEBUG AUTO-WATER: STARTED by low mat humidity (%.1f%% < %.1f%%)! Cycle %d (unix %lu)\n", mat.humidity, sett.matMinHumidity - 10.0, sett.cycleCount, nowUnix);
                    } else {
                      LOG("DEBUG AUTO-WATER: Conditions not met - radSum=%.2f < %.2f J/m2 (maxInt=%.2f h), matHum=%.1f%% >= %.1f%%\n", sett.radSum, sett.radThreshold * 1e6, (float)elapsedWater / 3600.0, mat.humidity, sett.matMinHumidity);
                    }
                  } else {
                    LOG("DEBUG AUTO-WATER: Mat EC high (%.2f >= %.2f) - no watering\n", mat.ec, sett.matMaxEC);
                  }
                } else {
                  LOG("DEBUG AUTO-WATER: Mat humidity low or invalid (%.1f%% <= %.1f%%) - no watering\n", mat.humidity, sett.matMinHumidity);
                }
              } else {
                LOG("DEBUG AUTO-WATER: Min interval not elapsed (%u sec < %u sec) - skip\n", elapsedWater, minSec);
              }
            } else {
              LOG("DEBUG AUTO-WATER: Max cycles reached (%d/%d) - skip\n", sett.cycleCount, sett.maxWateringCycles);
            }
          } else {
            LOG("DEBUG AUTO-WATER: Low level (%.1f%% <10) - skip\n", level);
          }
        } else {
          LOG("DEBUG AUTO-WATER: Outside watering hours (current %02d:%02d) - skip\n", h, m);
        }
      }
      uint32_t elapsedWater = nowUnix - sett.lastWateringStartUnix;
      uint32_t pumpSec = sett.pumpTime / 1000UL;
      if (sett.pumpState && elapsedWater >= pumpSec) {
        sett.pumpState = false;
        needSave = true;
        saveCaller = "auto_watering_ended";
        LOG("DEBUG AUTO-WATER: Auto watering ended after %u sec (unix %lu)\n", elapsedWater, nowUnix);
      }
    }
    if (sett.wateringMode == 1 && !sett.forceWateringActive && !sett.forceWateringOverride && !sett.morningWateringActive) {
      static uint32_t lastManualPeriodicCheck = 0;
      if (millis() - lastManualPeriodicCheck >= 60000) {
        lastManualPeriodicCheck = millis();
        if (currentMin >= startMin && currentMin < endMin) {
          float level;
          xQueuePeek(qLevel, &level, 0);
          uint32_t elapsedManual = nowUnix - sett.lastManualWateringTimeUnix;
          uint32_t intervalSec = sett.manualWateringInterval / 1000UL;
          if (elapsedManual >= intervalSec && level >= 10) {
            sett.lastManualWateringTimeUnix = nowUnix;
            sett.lastWateringStartUnix = nowUnix;
            sett.radSum = 0.0f;
            sett.lastSunTimeUnix = nowUnix;
            sett.pumpState = true;
            sett.cycleCount++;
            needSave = true;
            saveCaller = "manual_watering_started";
            LOG("DEBUG MANUAL-WATER: Started periodic watering, Cycle %d (unix %lu)\n", sett.cycleCount, nowUnix);
          }
        }
      }
      uint32_t manualSec = sett.manualWateringDuration / 1000UL;
      if (sett.pumpState && (nowUnix - sett.lastWateringStartUnix) >= manualSec) {
        sett.pumpState = false;
        needSave = true;
        saveCaller = "manual_watering_ended";
        LOG("DEBUG MANUAL-WATER: Manual watering ended after %u sec (unix %lu)\n", manualSec, nowUnix);
      }
    }
    bool targetPumpState = false;
    bool targetThreeWayState = false;
    bool targetFillState = sett.fillState;
    float currentLevel;
    xQueuePeek(qLevel, &currentLevel, 0);
    bool shouldTransitionToNormal = false;
    int transitionMode = sett.prevWateringMode; // По умолчанию к предыдущему
    if (sett.forceWateringActive) {
      if (sett.hydroMix) {
        uint32_t hydroSec = sett.hydroMixDuration / 1000UL;
        if ((nowUnix - sett.hydroStartUnix) < hydroSec) {
          targetPumpState = true;
          targetThreeWayState = true;
          if (!forcedPumpStarted) {
            forcedPumpStarted = true;
            sett.forcedWateringPerformed = true;
            sett.lastWateringStartUnix = nowUnix;
            LOG("DEBUG: Hydro mix active - REL_PUMP HIGH, REL_3WAY HIGH, timers reset\n");
          } else {
            LOG("DEBUG: Hydro mix active - REL_PUMP HIGH, REL_3WAY HIGH\n");
          }
        } else {
          sett.hydroMix = false;
          sett.hydroStartUnix = 0;
          LOG("DEBUG: Hydro mix timeout - continue fill if active\n");
          if (!sett.fillState) {
            // Если нет fill, но был hydro — переходить, если таймаут основной истёк
            if (nowUnix >= sett.forceWateringEndTimeUnix) {
              shouldTransitionToNormal = true;
            }
          } // Иначе продолжить fill
        }
      }
      if (sett.fillState) {
        if (currentLevel >= 99.0) {
          sett.fillState = false;
          LOG("DEBUG: Fill complete - check for transition\n");
          // Если был hydro (даже завершённый), теперь переходить
          if (!sett.hydroMix) { // Hydro уже завершён или не был
            shouldTransitionToNormal = true;
          }
          // Если hydro ещё идёт, продолжить (но hydro таймаут выше)
        } else {
          targetFillState = true;
          // Для fill без hydro: pump только если нужно, но без сброса таймера (fill не считается поливом)
          targetPumpState = sett.hydroMix; // Насос только если hydro
          targetThreeWayState = sett.hydroMix;
          LOG("DEBUG: Fill active - REL_FILL HIGH (level %.1f%%)\n", currentLevel);
        }
      } else {
        // Нет fill и нет hydro — стандартный таймаут
        targetPumpState = (nowUnix < sett.forceWateringEndTimeUnix);
        if (targetPumpState && !forcedPumpStarted) {
          forcedPumpStarted = true;
          sett.forcedWateringPerformed = true;
          sett.lastWateringStartUnix = nowUnix;
          LOG("DEBUG: Forced pump start - timers reset\n");
        }
        if (!targetPumpState && !forcedPumpStarted && nowUnix >= sett.forceWateringEndTimeUnix) {
          // Если не стартовал pump, не засчитывать
          shouldTransitionToNormal = true;
        }
      }
      if (shouldTransitionToNormal) {
        int targetMode = sett.prevWateringMode;
        if (!sett.forcedWateringPerformed) {
          targetMode = 2;
        }
        digitalWrite(REL_PUMP, LOW);
        sett.pumpState = false;
        sett.forceWateringActive = false;
        sett.wateringMode = targetMode;
        // Сброс таймеров только если был performed
        if (sett.forcedWateringPerformed) {
          sett.lastWateringStartUnix = nowUnix;
          if (targetMode == 0) {
            sett.radSum = 0.0f;
            sett.cycleCount++;
          } else if (targetMode == 1) {
            sett.lastManualWateringTimeUnix = nowUnix;
          }
          sett.forcedWateringPerformed = false;
        }
        sett.forceWateringOverride = false;
        sett.forceWateringEndTimeUnix = 0;
        // Обработка morning
        if (targetMode == 0 || targetMode == 1) {
          if (sett.morningWateringActive) {
            sett.lastMorningWateringEndUnix = nowUnix;
            if (sett.pendingMorningComplete) {
              sett.currentMorningWatering++;
              sett.pendingMorningComplete = false;
              LOG("DEBUG MORNING-WATER: Forced counted as completed morning watering #%u\n", sett.currentMorningWatering);
            }
            if (sett.currentMorningWatering >= sett.morningWateringCount) {
              sett.morningWateringActive = false;
              // НЕ сбрасываем radSum
              sett.lastWateringStartUnix = sett.lastMorningWateringStartUnix;
              if (targetMode == 1) sett.lastManualWateringTimeUnix = sett.lastMorningWateringStartUnix;
              needSave = true;
              saveCaller = "morning_completed_after_forced";
              LOG("DEBUG MORNING-WATER: Sequence completed after forced! lastWateringStart=lastMorningStart\n");
              // Проверка на auto после forced+morning
              if (targetMode == 0 && sett.radSum >= (sett.radThreshold * 1e6)) {
                uint32_t minSec = sett.minWateringInterval / 1000UL;
                uint32_t elapsedFromLastMorning = nowUnix - sett.lastMorningWateringStartUnix;
                if (elapsedFromLastMorning >= minSec) {
                  sett.lastWateringStartUnix = nowUnix;
                  sett.cycleCount++;
                  sett.radSum = 0.0f;
                  sett.lastSunTimeUnix = nowUnix;
                  sett.pumpState = true;
                  needSave = true;
                  saveCaller = "auto_after_forced_morning_rad";
                  LOG("DEBUG AUTO-WATER: STARTED after forced+morning by radiation! Cycle %d (unix %lu)\n", sett.cycleCount, nowUnix);
                }
              }
            }
          }
        }
        needSave = true;
        saveCaller = "force_watering_transition";
        LOG("DEBUG: Force watering transition to mode %d after completion (unix %lu, performed=%s)\n", targetMode, nowUnix, sett.forcedWateringPerformed ? "true" : "false");
        publishAllSensors(); // Явная публикация после перехода
      }
    } else if (sett.forceWateringOverride) {
      targetPumpState = false;
      targetThreeWayState = false;
      targetFillState = false;
      LOG("DEBUG: Override active - all watering OFF\n");
    } else {
      targetPumpState = sett.pumpState;
    }
    if (digitalRead(REL_PUMP) != targetPumpState) {
      digitalWrite(REL_PUMP, targetPumpState ? HIGH : LOW);
      sett.pumpState = targetPumpState;
      needSave = true;
      saveCaller = targetPumpState ? "pump_turned_on" : "pump_turned_off";
    }
    if (digitalRead(REL_3WAY) != (targetThreeWayState ? HIGH : LOW)) {
      digitalWrite(REL_3WAY, targetThreeWayState ? HIGH : LOW);
      LOG("DEBUG: REL_3WAY set to %s\n", targetThreeWayState ? "HIGH" : "LOW");
    }
    static uint32_t lastFillChange = 0;
    if (digitalRead(REL_FILL) != (targetFillState ? HIGH : LOW) && millis() - lastFillChange > 5000) {
      digitalWrite(REL_FILL, targetFillState ? HIGH : LOW);
      lastFillChange = millis();
      LOG("DEBUG: REL_FILL set to %s\n", targetFillState ? "HIGH" : "LOW");
    }
    if (isnan(outdoorTemp)) outdoorTemp = 0;
    if (isnan(outdoorHum)) outdoorHum = 0;
    if (isnan(avgHum)) avgHum = 0;
    if (isnan(sol)) sol = 0;
    if (isnan(level)) level = 0;
    if (isnan(wind)) wind = 0;
    if (millis() - lastHistory > 60000) {
      addToHistory(avgTemp, avgHum);
      lastHistory = millis();
    }
    if (wind > sett.wind1 || wind > sett.wind2) {
      windLockStart = millis();
      windRestricted = true;
    }
    if (windLockStart && millis() - windLockStart > sett.windLockMinutes * 60 * 1000) {
      windRestricted = false;
      windLockStart = 0;
    }
    bool windChanged = (windRestricted != prevWindRestricted);
    prevWindRestricted = windRestricted;
    bool shouldRecalcWindow = false;
    if (!sett.manualWindow && sensorsValid) {
      if (isnan(lastAvgTemp) || isnan(lastAvgHum) || abs(avgTemp - lastAvgTemp) >= tempHysteresis || abs(avgHum - lastAvgHum) >= humHysteresis) {
        shouldRecalcWindow = true;
      }
      if (windChanged) {
        shouldRecalcWindow = true;
        Serial.printf("Wind restriction changed to %s - immediate window recalculation\n", windRestricted ? "true" : "false");
      }
      if (shouldRecalcWindow) {
        lastAvgTemp = avgTemp;
        lastAvgHum = avgHum;
        float effectiveMinTemp = sett.minTemp;
        float effectiveMaxTemp = sett.maxTemp;
        if (isNight()) {
          effectiveMinTemp -= sett.nightOffset;
          effectiveMaxTemp -= sett.nightOffset;
        }
        float tTarget = mapFloat(avgTemp, effectiveMinTemp, effectiveMaxTemp, 0, 100);
        float hTarget = mapFloat(avgHum, sett.minHum, sett.maxHum, 100, 0);
        int target = max(tTarget, hTarget);
        if (avgTemp < effectiveMinTemp && hTarget > tTarget) target = 0;
        if (windRestricted) {
          if (wind > sett.wind2 && avgTemp < 27) target = 0;
          else if (wind > sett.wind2) target = min(target, 5);
          else if (wind > sett.wind1) target = min(target, 20);
        }
        target = constrain(target, 0, 100);
        if (windowPos != target && !windowMoving && (millis() - lastWindowMoveCommand > minWindowStateChangeInterval)) {
          int delta = abs(target - windowPos);
          uint32_t fullTime = (target > windowPos) ? sett.openTime : sett.closeTime;
          uint32_t duration = (delta / 100.0f) * fullTime;
          String dir = (target > windowPos) ? "up" : "down";
          if (dir == "down" && target == 0) {
            duration += 5000;
            Serial.println("Добавляем 5 сек extra для полного закрытия форточки");
          }
          startWindowMovement(dir, duration);
          targetWindowPos = target;
          lastWindowMoveCommand = millis();
        }
      }
    }
    if (windowMoving && !sett.manualWindow && millis() - windowStartTime >= moveDuration) {
      stopWindowMovement();
      windowPos = targetWindowPos;
      Serial.printf("Auto window movement completed. New position: %d%%\n", windowPos);
      if (client.connected()) {
        client.publish("greenhouse/esp32/window", String(windowPos).c_str(), true);
      }
    }
    if (!sett.manualHeat && sensorsValid && millis() - lastHeatChange > minStateChangeInterval) {
      float on = sett.heatOn, off = sett.heatOff;
      if (isNight()) {
        on -= sett.nightOffset;
        off -= sett.nightOffset;
      }
      bool newHeatState = heat;
      if (avgTemp <= on && !heat) {
        newHeatState = true;
        Serial.println("Heating turned ON");
        lastHeatChange = millis();
      } else if (avgTemp >= off && heat) {
        newHeatState = false;
        Serial.println("Heating turned OFF");
        lastHeatChange = millis();
      }
      if (sett.heatState != newHeatState) {
        sett.heatState = newHeatState;
        heat = newHeatState;
        needSave = true;
        saveCaller = "heating";
      }
      digitalWrite(REL_HEAT, newHeatState ? HIGH : LOW);
    } else if (!sensorsValid && heat) {
      digitalWrite(REL_HEAT, LOW);
      heat = false;
      if (sett.heatState != false) {
        sett.heatState = false;
        needSave = true;
        saveCaller = "heating_no_sensors";
      }
    } else if (sett.manualHeat) {
      digitalWrite(REL_HEAT, sett.heatState ? HIGH : LOW);
      heat = sett.heatState;
    }
    if (!sett.manualSol && level > 20 && sensorsValid && millis() - lastSolHeatChange > minStateChangeInterval) {
      float on = sett.solOn, off = sett.solOff;
      if (isNight()) {
        on -= sett.nightOffset;
        off -= sett.nightOffset;
      }
      bool newSolHeatState = solHeat;
      if (sol <= on && !solHeat) {
        newSolHeatState = true;
        Serial.println("Solution heating turned ON");
        lastSolHeatChange = millis();
      } else if (sol >= off && solHeat) {
        newSolHeatState = false;
        Serial.println("Solution heating turned OFF");
        lastSolHeatChange = millis();
      }
      if (sett.solHeatState != newSolHeatState) {
        sett.solHeatState = newSolHeatState;
        solHeat = newSolHeatState;
        needSave = true;
        saveCaller = "solution_heating";
      }
      digitalWrite(REL_SOL, newSolHeatState ? HIGH : LOW);
    } else if (!sensorsValid && solHeat) {
      digitalWrite(REL_SOL, LOW);
      solHeat = false;
      if (sett.solHeatState != false) {
        sett.solHeatState = false;
        needSave = true;
        saveCaller = "solution_heating_no_sensors";
      }
    } else if (sett.manualSol) {
      digitalWrite(REL_SOL, sett.solHeatState ? HIGH : LOW);
      solHeat = sett.solHeatState;
    }
    bool targetFan = fan;
    static uint32_t fanStartTimestamp = 0;
    if (sett.manualFan) {
      targetFan = sett.fanState;
    } else {
      bool shouldTurnFanOnForFog = false;
      if (!fan && fog) {
        shouldTurnFanOnForFog = true;
      }
      bool shouldTurnFanOnBySchedule = false;
      if (!fan && !fog) {
        if (millis() - lastFanChange >= sett.fanInterval) {
          shouldTurnFanOnBySchedule = true;
        }
      }
      bool shouldTurnFanOffByDuration = false;
      if (fan) {
        if (millis() - fanStartTimestamp >= sett.fanDuration) {
          shouldTurnFanOffByDuration = true;
        }
      }
      if (shouldTurnFanOnForFog) {
        targetFan = true;
      } else if (shouldTurnFanOffByDuration) {
        targetFan = false;
      } else if (shouldTurnFanOnBySchedule) {
        targetFan = true;
      }
    }
    if (targetFan != fan) {
      fan = targetFan;
      digitalWrite(REL_FAN, fan ? HIGH : LOW);
      if (fan) {
        fanStartTimestamp = millis();
      }
      lastFanChange = millis();
      if (sett.fanState != fan) {
        sett.fanState = fan;
        needSave = true;
        saveCaller = "fan_state_changed_by_logic";
        LOG("DEBUG: Fan state changed to %s. lastFanChange updated to %lu\n", fan ? "ON" : "OFF", lastFanChange);
      }
    }
    h = nowTime.hour();
    m = nowTime.minute();
    bool morning = h >= sett.fogMorningStart && h < sett.fogMorningEnd;
    bool day = h >= sett.autoFogDayStart && h < sett.autoFogDayEnd;
    uint32_t dur = 0, intv = 0;
    bool shouldRun = false;
    if (sett.fogMode == 2) {
      if (sett.forceFogOn && !fog) {
        if (!fogValveOn) {
          digitalWrite(REL_FOG_VALVE, HIGH);
          fogValveOn = true;
          fogValveStartTime = millis();
          Serial.println("Fog valve turned ON in forced mode");
        } else if (millis() - fogValveStartTime >= sett.fogDelay * 1000) {
          digitalWrite(REL_FOG_PUMP, HIGH);
          fog = true;
          fogValveOn = false;
          if (sett.fogState != true) {
            sett.fogState = true;
            needSave = true;
            saveCaller = "fog_forced";
          }
        }
      } else if (!sett.forceFogOn && fog) {
        digitalWrite(REL_FOG_PUMP, LOW);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        digitalWrite(REL_FOG_VALVE, LOW);
        fog = false;
        fogValveOn = false;
        if (sett.fogState != false) {
          sett.fogState = false;
          needSave = true;
          saveCaller = "fog_forced";
        }
      }
    } else if (sett.fogMode == 1) {
      if (morning) {
        dur = sett.fogMorningDuration;
        intv = sett.fogMorningInterval;
      } else if (day) {
        dur = sett.fogDayDuration;
        intv = sett.fogDayInterval;
      }
      if ((morning || day) && millis() - taskLastFog > intv && !fog) {
        if (!fogValveOn) {
          digitalWrite(REL_FOG_VALVE, HIGH);
          fogValveOn = true;
          fogValveStartTime = millis();
          Serial.println("Fog valve turned ON in manual mode");
        } else if (millis() - fogValveStartTime >= sett.fogDelay * 1000) {
          digitalWrite(REL_FOG_PUMP, HIGH);
          fog = true;
          fogValveOn = false;
          if (sett.fogState != true) {
            sett.fogState = true;
            taskFogStartTime = millis();
            needSave = true;
            saveCaller = "fog_manual";
          }
        }
      }
      if (fog && millis() - taskFogStartTime > dur) {
        digitalWrite(REL_FOG_PUMP, LOW);
        digitalWrite(REL_FOG_VALVE, LOW);
        fog = false;
        fogValveOn = false;
        if (sett.fogState != false) {
          sett.fogState = false;
          taskLastFog = millis();
          needSave = true;
          saveCaller = "fog_manual";
        }
      }
    } else if (sett.fogMode == 0) {
      if (morning) {
        dur = sett.fogMorningDuration;
        intv = sett.fogMorningInterval;
        shouldRun = true;
      } else if (day) {
        float hum = avgHum, temp = avgTemp;
        if (isnan(hum) || isnan(temp)) {
          hum = sett.fogMinHum + 1;
          temp = (sett.fogMinTemp + sett.fogMaxTemp) / 2;
          Serial.println("Warning: Invalid avgHum or avgTemp, using defaults");
        }
        dur = calcFogDuration(hum, temp);
        intv = calcFogInterval(hum, temp);
        shouldRun = (hum < sett.fogMaxHum && temp >= sett.fogMinTemp && temp <= sett.fogMaxTemp);
      }
      if (shouldRun && millis() - taskLastFog > intv && !fog) {
        if (!fogValveOn) {
          digitalWrite(REL_FOG_VALVE, HIGH);
          fogValveOn = true;
          fogValveStartTime = millis();
          Serial.println("Fog valve turned ON in auto mode");
        } else if (millis() - fogValveStartTime >= sett.fogDelay * 1000) {
          digitalWrite(REL_FOG_PUMP, HIGH);
          fog = true;
          fogValveOn = false;
          if (sett.fogState != true) {
            sett.fogState = true;
            taskFogStartTime = millis();
            needSave = true;
            saveCaller = "fog_auto";
          }
        }
      }
      if (fog && millis() - taskFogStartTime > dur) {
        digitalWrite(REL_FOG_PUMP, LOW);
        digitalWrite(REL_FOG_VALVE, LOW);
        fog = false;
        fogValveOn = false;
        if (sett.fogState != false) {
          sett.fogState = false;
          taskLastFog = millis();
          needSave = true;
          saveCaller = "fog_auto";
        }
      }
    }
    if ((sett.fogMode == 0 || sett.fogMode == 1) && !(morning || day) && fog) {
      digitalWrite(REL_FOG_PUMP, LOW);
      digitalWrite(REL_FOG_VALVE, LOW);
      fog = false;
      fogValveOn = false;
      if (sett.fogState != false) {
        sett.fogState = false;
        needSave = true;
        saveCaller = "fog_outside_window";
      }
    }
    if (needSave && strcmp(saveCaller, "unknown") != 0) {
      saveSettings(saveCaller);
      publishSettings();
      publishAllSensors(); // Явная публикация после save
    }
    static uint32_t mqttRetryInterval = client.connected() ? 60000UL : 10000UL;
    if (now - lastMqttCheck > mqttRetryInterval) {
      if (!client.connected()) {
        Serial.println("MQTT disconnected → reconnecting...");
        bool reconnectSuccess = false;
        unsigned long connectStart = millis();
        while (millis() - connectStart < 3000UL && !reconnectSuccess) {
          if (client.connect("ESP32_Greenhouse")) {
            client.subscribe("greenhouse/set_settings");
            client.subscribe("greenhouse/cmd/#");
            client.publish("greenhouse/esp32/status", "online", true);
            Serial.println("MQTT reconnected!");
            reconnectSuccess = true;
            publishSettings();
            publishAllSensors(); // Публикация после reconnect
          } else {
            Serial.printf("MQTT reconnect attempt failed, rc=%d\n", client.state());
            vTaskDelay(100 / portTICK_PERIOD_MS);
          }
        }
        if (!reconnectSuccess) {
          Serial.println("MQTT reconnect timed out after 3s — will retry soon");
        }
      } else {
        client.loop();
      }
      lastMqttCheck = now;
      mqttRetryInterval = client.connected() ? 60000UL : 10000UL;
    } else {
      client.loop();
    }
    static uint32_t lastPub = 0;
    if (client.connected() && (millis() - lastPub > 5000UL)) {
      publishAllSensors();
      lastPub = millis();
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC DS3231");
    delay(2000);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time from compilation (__DATE__ and __TIME__)");
    char dateStr[] = __DATE__;
    char timeStr[] = __TIME__;
    int month = 1;
    if (strncmp(dateStr, "Jan", 3) == 0) month = 1;
    else if (strncmp(dateStr, "Feb", 3) == 0) month = 2;
    else if (strncmp(dateStr, "Mar", 3) == 0) month = 3;
    else if (strncmp(dateStr, "Apr", 3) == 0) month = 4;
    else if (strncmp(dateStr, "May", 3) == 0) month = 5;
    else if (strncmp(dateStr, "Jun", 3) == 0) month = 6;
    else if (strncmp(dateStr, "Jul", 3) == 0) month = 7;
    else if (strncmp(dateStr, "Aug", 3) == 0) month = 8;
    else if (strncmp(dateStr, "Sep", 3) == 0) month = 9;
    else if (strncmp(dateStr, "Oct", 3) == 0) month = 10;
    else if (strncmp(dateStr, "Nov", 3) == 0) month = 11;
    else if (strncmp(dateStr, "Dec", 3) == 0) month = 12;
    int day = atoi(dateStr + 4);
    int year = atoi(dateStr + 7);
    int hour = atoi(timeStr);
    int minute = atoi(timeStr + 3);
    int second = atoi(timeStr + 6);
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    Serial.printf("RTC set to compilation time: %02d:%02d:%02d %02d/%02d/%04d\n",
                  hour, minute, second, day, month, year);
  }
  DateTime now = rtc.now();
  Serial.printf("Current RTC time: %02d:%02d:%02d %02d/%02d/%04d (epoch: %lu)\n", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year(), now.unixtime());
  LittleFS.begin();
  RS485.begin(MODBUS_BAUD, SERIAL_8N1, MODBUS_RX, MODBUS_TX);
  mb.begin(1, RS485);
  Serial.println("ModbusMaster initialized.");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
    delay(500);
    Serial.print(".");
  }
  delay(2000);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi initially connected in setup!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    wifiConnectedOnce = true;
    timeClient.begin();
    timeClient.update();
    delay(2000);
    unsigned long syncStart = millis();
    while ((rtc.now().unixtime() <= 1704067200UL) && (millis() - syncStart < 30000)) {
      if (timeClient.update()) {
        unsigned long epoch = timeClient.getEpochTime();
        if (epoch > 1704067200UL) {
          rtc.adjust(DateTime(epoch));
          Serial.println("Initial NTP sync OK, RTC updated");
          break;
        }
      }
      Serial.print("NTP retry... ");
      delay(2000);
    }
    if (rtc.now().unixtime() <= 1704067200UL) {
      Serial.println("WARNING: Initial NTP failed — RTC remains at default time!");
    } else {
      Serial.printf("Initial RTC time after sync: %s (epoch: %lu)\n", timeClient.getFormattedTime().c_str(), rtc.now().unixtime());
    }
  } else {
    Serial.println("\nFailed to connect to Wi-Fi initially in setup. Will retry in task. RTC uses default time.");
  }
  client.setServer(MQTT_BROKER, 1883);
  client.setCallback(callback);
  client.setSocketTimeout(MQTT_CONNECT_TIMEOUT / 1000);
  if (client.connect("ESP32_Greenhouse")) {
    client.subscribe("greenhouse/set_settings");
    client.subscribe("greenhouse/cmd/#");
    Serial.println("Connected to MQTT broker");
    client.publish("greenhouse/esp32/status", "online", true);
    publishSettings();
  } else {
    Serial.println("Failed to connect to MQTT broker");
  }
  delay(2000);
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
  digitalWrite(REL_WINDOW_DN, HIGH);
  delay(156000 + 5000);
  digitalWrite(REL_WINDOW_DN, LOW);
  windowPos = 0;
  Serial.println("Window calibrated to 0% with extra close");
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }
  loadSettings();
  loadHistory();
  qAvg = xQueueCreate(1, sizeof(SensorData));
  qSol = xQueueCreate(1, sizeof(float));
  qLevel = xQueueCreate(1, sizeof(float));
  qWind = xQueueCreate(1, sizeof(float));
  qOutdoor = xQueueCreate(1, sizeof(SensorData));
  qPyrano = xQueueCreate(1, sizeof(float));
  qMat = xQueueCreate(1, sizeof(MatSensorData));
  mtx = xSemaphoreCreateMutex();
  networkTimeTaskHandle = NULL;
  xTaskCreate(modbusTask, "modbus", 4096, NULL, 1, NULL);
  xTaskCreatePinnedToCore(logicTask, "logic", 12288, NULL, 1, NULL, 0);
  xTaskCreate(networkTimeManagementTask, "Network & Time Mgmt", 4096, NULL, 1, &networkTimeTaskHandle);
  if (networkTimeTaskHandle != NULL) {
    Serial.println("Network & Time Management Task created successfully");
  } else {
    Serial.println("Failed to create Network & Time Management Task");
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(4096);
    doc["minTemp"] = sett.minTemp;
    doc["maxTemp"] = sett.maxTemp;
    doc["minHum"] = sett.minHum;
    doc["maxHum"] = sett.maxHum;
    doc["openTime"] = sett.openTime;
    doc["closeTime"] = sett.closeTime;
    doc["heatOn"] = sett.heatOn;
    doc["heatOff"] = sett.heatOff;
    doc["fanInterval"] = sett.fanInterval;
    doc["fanDuration"] = sett.fanDuration;
    doc["fogMorningStart"] = sett.fogMorningStart;
    doc["fogMorningEnd"] = sett.fogMorningEnd;
    doc["fogDayStart"] = sett.fogDayStart;
    doc["fogDayEnd"] = sett.fogDayEnd;
    doc["fogMorningDuration"] = sett.fogMorningDuration;
    doc["fogMorningInterval"] = sett.fogMorningInterval;
    doc["fogDayDuration"] = sett.fogDayDuration;
    doc["fogDayInterval"] = sett.fogDayInterval;
    doc["fogDelay"] = sett.fogDelay;
    doc["fogMinHum"] = sett.fogMinHum;
    doc["fogMaxHum"] = sett.fogMaxHum;
    doc["fogMinTemp"] = sett.fogMinTemp;
    doc["fogMaxTemp"] = sett.fogMaxTemp;
    doc["fogMinDuration"] = sett.fogMinDuration;
    doc["fogMaxDuration"] = sett.fogMaxDuration;
    doc["fogMinInterval"] = sett.fogMinInterval;
    doc["fogMaxInterval"] = sett.fogMaxInterval;
    doc["solOn"] = sett.solOn;
    doc["solOff"] = sett.solOff;
    doc["wind1"] = sett.wind1;
    doc["wind2"] = sett.wind2;
    doc["windLockMinutes"] = sett.windLockMinutes;
    doc["nightStart"] = sett.nightStart;
    doc["dayStart"] = sett.dayStart;
    doc["nightOffset"] = sett.nightOffset;
    doc["manualWindow"] = sett.manualWindow;
    doc["manualHeat"] = sett.manualHeat;
    doc["manualPump"] = sett.manualPump;
    doc["manualSol"] = sett.manualSol;
    doc["fogMode"] = sett.fogMode;
    doc["manualFan"] = sett.manualFan;
    doc["hydroMixDuration"] = sett.hydroMixDuration;
    doc["previousManualPump"] = sett.previousManualPump;
    doc["manualPumpOverride"] = sett.manualPumpOverride;
    doc["forceFogOn"] = sett.forceFogOn;
    doc["heatState"] = sett.heatState;
    doc["fanState"] = sett.fanState;
    doc["fogState"] = sett.fogState;
    doc["pumpState"] = sett.pumpState;
    doc["solHeatState"] = sett.solHeatState;
    doc["autoFogDayStart"] = sett.autoFogDayStart;
    doc["autoFogDayEnd"] = sett.autoFogDayEnd;
    doc["levelMin"] = sett.levelMin;
    doc["levelMax"] = sett.levelMax;
    doc["hydroMix"] = sett.hydroMix;
    doc["hydroStart"] = sett.hydroStart;
    doc["fillState"] = sett.fillState;
    JsonArray modes = doc.createNestedArray("wateringModes");
    for (int i = 0; i < 4; i++) {
      JsonObject mode = modes.createNestedObject();
      mode["enabled"] = sett.wateringModes[i].enabled;
      mode["startHour"] = sett.wateringModes[i].startHour;
      mode["endHour"] = sett.wateringModes[i].endHour;
      mode["duration"] = sett.wateringModes[i].duration;
    }
    doc["radThreshold"] = sett.radThreshold;
    doc["pumpTime"] = sett.pumpTime;
    doc["wateringStartHour"] = sett.wateringStartHour;
    doc["wateringStartMinute"] = sett.wateringStartMinute;
    doc["wateringEndHour"] = sett.wateringEndHour;
    doc["wateringEndMinute"] = sett.wateringEndMinute;
    doc["maxWateringCycles"] = sett.maxWateringCycles;
    doc["radSumResetInterval"] = sett.radSumResetInterval;
    doc["minWateringInterval"] = sett.minWateringInterval;
    doc["wateringMode"] = sett.wateringMode;
    doc["manualWateringInterval"] = round(sett.manualWateringInterval / (60.0 * 1000UL));
    doc["manualWateringDuration"] = sett.manualWateringDuration / 1000UL;
    doc["matMinHumidity"] = sett.matMinHumidity;
    doc["matMaxEC"] = sett.matMaxEC;
    doc["maxWateringInterval"] = sett.maxWateringInterval;
    doc["forcedWateringDuration"] = sett.forcedWateringDuration;
    doc["morningWateringCount"] = sett.morningWateringCount;
    doc["morningWateringInterval"] = round(sett.morningWateringInterval / (60.0 * 1000UL));
    doc["morningWateringDuration"] = sett.morningWateringDuration / 1000UL;
    doc["forcedWateringPerformed"] = sett.forcedWateringPerformed;
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });
  server.on(
    "/settings", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      static uint8_t jsonBuffer[4096];
      static size_t bufferIndex = 0;
      if (index + len >= sizeof(jsonBuffer)) {
        Serial.println("JSON data too large for buffer");
        request->send(413, "application/json", "{\"error\":\"Request Entity Too Large\"}");
        bufferIndex = 0;
        return;
      }
      memcpy(jsonBuffer + index, data, len);
      bufferIndex = index + len;
      if (bufferIndex == total) {
        jsonBuffer[bufferIndex] = '\0';
        DynamicJsonDocument doc(5120);
        DeserializationError error = deserializeJson(doc, (const char *)jsonBuffer, bufferIndex);
        if (error) {
          Serial.printf("JSON parsing error: %s\n", error.c_str());
          String errorMsg = "{\"error\":\"Invalid JSON: ";
          errorMsg += error.c_str();
          errorMsg += "\"}";
          request->send(400, "application/json", errorMsg.c_str());
          bufferIndex = 0;
          return;
        }
        bool needSave = false;
        const char *saveCaller = "http_settings";
        if (doc.containsKey("minTemp")) {
          float val = doc["minTemp"];
          if (sett.minTemp != val) {
            sett.minTemp = val;
            needSave = true;
          }
        }
        if (doc.containsKey("maxTemp")) {
          float val = doc["maxTemp"];
          if (sett.maxTemp != val) {
            sett.maxTemp = val;
            needSave = true;
          }
        }
        if (doc.containsKey("minHum")) {
          float val = doc["minHum"];
          if (sett.minHum != val) {
            sett.minHum = val;
            needSave = true;
          }
        }
        if (doc.containsKey("maxHum")) {
          float val = doc["maxHum"];
          if (sett.maxHum != val) {
            sett.maxHum = val;
            needSave = true;
          }
        }
        if (doc.containsKey("openTime")) {
          uint32_t val = doc["openTime"];
          if (sett.openTime != val) {
            sett.openTime = val;
            needSave = true;
          }
        }
        if (doc.containsKey("closeTime")) {
          uint32_t val = doc["closeTime"];
          if (sett.closeTime != val) {
            sett.closeTime = val;
            needSave = true;
          }
        }
        if (doc.containsKey("heatOn")) {
          float val = doc["heatOn"];
          if (sett.heatOn != val) {
            sett.heatOn = val;
            needSave = true;
          }
        }
        if (doc.containsKey("heatOff")) {
          float val = doc["heatOff"];
          if (sett.heatOff != val) {
            sett.heatOff = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fanInterval")) {
          uint32_t val = doc["fanInterval"];
          if (sett.fanInterval != val) {
            sett.fanInterval = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fanDuration")) {
          uint32_t val = doc["fanDuration"];
          if (sett.fanDuration != val) {
            sett.fanDuration = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMorningStart")) {
          int val = doc["fogMorningStart"];
          if (sett.fogMorningStart != val) {
            sett.fogMorningStart = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMorningEnd")) {
          int val = doc["fogMorningEnd"];
          if (sett.fogMorningEnd != val) {
            sett.fogMorningEnd = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogDayStart")) {
          int val = doc["fogDayStart"];
          if (sett.fogDayStart != val) {
            sett.fogDayStart = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogDayEnd")) {
          int val = doc["fogDayEnd"];
          if (sett.fogDayEnd != val) {
            sett.fogDayEnd = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMorningDuration")) {
          uint32_t val = doc["fogMorningDuration"];
          if (sett.fogMorningDuration != val) {
            sett.fogMorningDuration = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMorningInterval")) {
          uint32_t val = doc["fogMorningInterval"];
          if (sett.fogMorningInterval != val) {
            sett.fogMorningInterval = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogDayDuration")) {
          uint32_t val = doc["fogDayDuration"];
          if (sett.fogDayDuration != val) {
            sett.fogDayDuration = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogDayInterval")) {
          uint32_t val = doc["fogDayInterval"];
          if (sett.fogDayInterval != val) {
            sett.fogDayInterval = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogDelay")) {
          uint32_t val = doc["fogDelay"];
          if (sett.fogDelay != val) {
            sett.fogDelay = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMinHum")) {
          float val = doc["fogMinHum"];
          if (sett.fogMinHum != val) {
            sett.fogMinHum = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMaxHum")) {
          float val = doc["fogMaxHum"];
          if (sett.fogMaxHum != val) {
            sett.fogMaxHum = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMinTemp")) {
          float val = doc["fogMinTemp"];
          if (sett.fogMinTemp != val) {
            sett.fogMinTemp = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMaxTemp")) {
          float val = doc["fogMaxTemp"];
          if (sett.fogMaxTemp != val) {
            sett.fogMaxTemp = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMinDuration")) {
          uint32_t val = doc["fogMinDuration"];
          if (sett.fogMinDuration != val) {
            sett.fogMinDuration = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMaxDuration")) {
          uint32_t val = doc["fogMaxDuration"];
          if (sett.fogMaxDuration != val) {
            sett.fogMaxDuration = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMinInterval")) {
          uint32_t val = doc["fogMinInterval"];
          if (sett.fogMinInterval != val) {
            sett.fogMinInterval = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMaxInterval")) {
          uint32_t val = doc["fogMaxInterval"];
          if (sett.fogMaxInterval != val) {
            sett.fogMaxInterval = val;
            needSave = true;
          }
        }
        if (doc.containsKey("solOn")) {
          float val = doc["solOn"];
          if (sett.solOn != val) {
            sett.solOn = val;
            needSave = true;
          }
        }
        if (doc.containsKey("solOff")) {
          float val = doc["solOff"];
          if (sett.solOff != val) {
            sett.solOff = val;
            needSave = true;
          }
        }
        if (doc.containsKey("wind1")) {
          float val = doc["wind1"];
          if (sett.wind1 != val) {
            sett.wind1 = val;
            needSave = true;
          }
        }
        if (doc.containsKey("wind2")) {
          float val = doc["wind2"];
          if (sett.wind2 != val) {
            sett.wind2 = val;
            needSave = true;
          }
        }
        if (doc.containsKey("windLockMinutes")) {
          uint32_t val = doc["windLockMinutes"];
          if (sett.windLockMinutes != val) {
            sett.windLockMinutes = val;
            needSave = true;
          }
        }
        if (doc.containsKey("nightStart")) {
          uint8_t val = doc["nightStart"];
          if (sett.nightStart != val) {
            sett.nightStart = val;
            needSave = true;
          }
        }
        if (doc.containsKey("dayStart")) {
          uint8_t val = doc["dayStart"];
          if (sett.dayStart != val) {
            sett.dayStart = val;
            needSave = true;
          }
        }
        if (doc.containsKey("nightOffset")) {
          float val = doc["nightOffset"];
          if (sett.nightOffset != val) {
            sett.nightOffset = val;
            needSave = true;
          }
        }
        if (doc.containsKey("manualWindow")) {
          bool val = doc["manualWindow"];
          if (sett.manualWindow != val) {
            sett.manualWindow = val;
            needSave = true;
          }
        }
        if (doc.containsKey("manualHeat")) {
          bool val = doc["manualHeat"];
          if (sett.manualHeat != val) {
            sett.manualHeat = val;
            needSave = true;
          }
        }
        if (doc.containsKey("manualPump")) {
          bool val = doc["manualPump"];
          if (sett.manualPump != val) {
            sett.manualPump = val;
            needSave = true;
          }
        }
        if (doc.containsKey("manualSol")) {
          bool val = doc["manualSol"];
          if (sett.manualSol != val) {
            sett.manualSol = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogMode")) {
          int val = doc["fogMode"];
          if (sett.fogMode != val) {
            sett.fogMode = val;
            needSave = true;
          }
        }
        if (doc.containsKey("manualFan")) {
          bool val = doc["manualFan"];
          if (sett.manualFan != val) {
            sett.manualFan = val;
            needSave = true;
          }
        }
        if (doc.containsKey("forceFogOn")) {
          bool val = doc["forceFogOn"];
          if (sett.forceFogOn != val) {
            sett.forceFogOn = val;
            needSave = true;
          }
        }
        if (doc.containsKey("autoFogDayStart")) {
          uint32_t val = doc["autoFogDayStart"];
          if (sett.autoFogDayStart != val) {
            sett.autoFogDayStart = val;
            needSave = true;
          }
        }
        if (doc.containsKey("autoFogDayEnd")) {
          uint32_t val = doc["autoFogDayEnd"];
          if (sett.autoFogDayEnd != val) {
            sett.autoFogDayEnd = val;
            needSave = true;
          }
        }
        if (doc.containsKey("levelMin")) {
          float val = doc["levelMin"];
          if (sett.levelMin != val) {
            sett.levelMin = val;
            needSave = true;
          }
        }
        if (doc.containsKey("levelMax")) {
          float val = doc["levelMax"];
          if (sett.levelMax != val) {
            sett.levelMax = val;
            needSave = true;
          }
        }
        if (doc.containsKey("hydroMixDuration")) {
          uint32_t val = doc["hydroMixDuration"];
          if (sett.hydroMixDuration != val) {
            sett.hydroMixDuration = val;
            needSave = true;
          }
        }
        if (doc.containsKey("previousManualPump")) {
          bool val = doc["previousManualPump"];
          if (sett.previousManualPump != val) {
            sett.previousManualPump = val;
            needSave = true;
          }
        }
        if (doc.containsKey("manualPumpOverride")) {
          bool val = doc["manualPumpOverride"];
          if (sett.manualPumpOverride != val) {
            sett.manualPumpOverride = val;
            needSave = true;
          }
        }
        if (doc.containsKey("heatState")) {
          bool val = doc["heatState"];
          if (sett.heatState != val) {
            sett.heatState = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fanState")) {
          bool val = doc["fanState"];
          if (sett.fanState != val) {
            sett.fanState = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fogState")) {
          bool val = doc["fogState"];
          if (sett.fogState != val) {
            sett.fogState = val;
            needSave = true;
          }
        }
        if (doc.containsKey("pumpState")) {
          bool val = doc["pumpState"];
          if (sett.pumpState != val) {
            sett.pumpState = val;
            needSave = true;
          }
        }
        if (doc.containsKey("solHeatState")) {
          bool val = doc["solHeatState"];
          if (sett.solHeatState != val) {
            sett.solHeatState = val;
            needSave = true;
          }
        }
        if (doc.containsKey("hydroMix")) {
          bool val = doc["hydroMix"];
          if (sett.hydroMix != val) {
            sett.hydroMix = val;
            needSave = true;
          }
        }
        if (doc.containsKey("hydroStart")) {
          uint32_t val = doc["hydroStart"];
          if (sett.hydroStart != val) {
            sett.hydroStart = val;
            needSave = true;
          }
        }
        if (doc.containsKey("fillState")) {
          bool val = doc["fillState"];
          if (sett.fillState != val) {
            sett.fillState = val;
            needSave = true;
          }
        }
        if (doc.containsKey("wateringModes")) {
          JsonArray wmArray = doc["wateringModes"];
          if (wmArray.size() == 4) {
            bool modesChanged = false;
            for (int i = 0; i < 4; i++) {
              JsonObject wm = wmArray[i];
              if (wm.containsKey("enabled")) {
                bool val = wm["enabled"];
                if (sett.wateringModes[i].enabled != val) {
                  sett.wateringModes[i].enabled = val;
                  modesChanged = true;
                }
              }
              if (wm.containsKey("startHour")) {
                uint8_t val = wm["startHour"];
                if (sett.wateringModes[i].startHour != val) {
                  sett.wateringModes[i].startHour = val;
                  modesChanged = true;
                }
              }
              if (wm.containsKey("endHour")) {
                uint8_t val = wm["endHour"];
                if (sett.wateringModes[i].endHour != val) {
                  sett.wateringModes[i].endHour = val;
                  modesChanged = true;
                }
              }
              if (wm.containsKey("duration")) {
                uint32_t val = wm["duration"];
                if (sett.wateringModes[i].duration != val) {
                  sett.wateringModes[i].duration = val;
                  modesChanged = true;
                }
              }
            }
            if (modesChanged) {
              needSave = true;
            }
          }
        }
        if (doc.containsKey("radCheckInterval")) {
          uint32_t val = doc["radCheckInterval"];
          if (sett.radCheckInterval != val) {
            sett.radCheckInterval = val;
            needSave = true;
          }
        }
        if (doc.containsKey("manualWateringInterval")) {
          uint64_t tempVal = (uint64_t)doc["manualWateringInterval"];
          uint32_t newVal = (uint32_t)tempVal;
          if (sett.manualWateringInterval != newVal) {
            sett.manualWateringInterval = newVal;
            needSave = true;
          } else {
            needSave = true;
          }
        }
        if (doc.containsKey("manualWateringDuration")) {
          uint64_t tempVal = (uint64_t)doc["manualWateringDuration"];
          uint32_t newVal = (uint32_t)tempVal;
          if (sett.manualWateringDuration != newVal) {
            sett.manualWateringDuration = newVal;
            needSave = true;
          } else {
            needSave = true;
          }
        }
        if (doc.containsKey("matMinHumidity")) {
          float val = doc["matMinHumidity"];
          if (sett.matMinHumidity != val) {
            sett.matMinHumidity = val;
            needSave = true;
          }
        }
        if (doc.containsKey("matMaxEC")) {
          float val = doc["matMaxEC"];
          if (sett.matMaxEC != val) {
            sett.matMaxEC = val;
            needSave = true;
          }
        }
        if (doc.containsKey("radThreshold")) {
          float val = doc["radThreshold"];
          if (sett.radThreshold != val) {
            sett.radThreshold = val;
            needSave = true;
          }
        }
        if (doc.containsKey("pumpTime")) {
          uint32_t val = doc["pumpTime"];
          if (sett.pumpTime != val) {
            sett.pumpTime = val;
            needSave = true;
          }
        }
        if (doc.containsKey("wateringStartHour")) {
          uint8_t val = doc["wateringStartHour"];
          if (sett.wateringStartHour != val) {
            sett.wateringStartHour = val;
            needSave = true;
          }
        }
        if (doc.containsKey("wateringStartMinute")) {
          uint8_t val = doc["wateringStartMinute"];
          if (sett.wateringStartMinute != val) {
            sett.wateringStartMinute = val;
            needSave = true;
          }
        }
        if (doc.containsKey("wateringEndHour")) {
          uint8_t val = doc["wateringEndHour"];
          if (sett.wateringEndHour != val) {
            sett.wateringEndHour = val;
            needSave = true;
          }
        }
        if (doc.containsKey("wateringEndMinute")) {
          uint8_t val = doc["wateringEndMinute"];
          if (sett.wateringEndMinute != val) {
            sett.wateringEndMinute = val;
            needSave = true;
          }
        }
        if (doc.containsKey("maxWateringCycles")) {
          uint8_t val = doc["maxWateringCycles"];
          if (sett.maxWateringCycles != val) {
            sett.maxWateringCycles = val;
            needSave = true;
          }
        }
        if (doc.containsKey("radSumResetInterval")) {
          uint32_t val = doc["radSumResetInterval"];
          if (sett.radSumResetInterval != val) {
            sett.radSumResetInterval = val;
            needSave = true;
          }
        }
        if (doc.containsKey("minWateringInterval")) {
          uint32_t val = doc["minWateringInterval"];
          if (sett.minWateringInterval != val) {
            sett.minWateringInterval = val;
            needSave = true;
          }
        }
        if (doc.containsKey("maxWateringInterval")) {
          uint32_t val = doc["maxWateringInterval"];
          if (sett.maxWateringInterval != val) {
            sett.maxWateringInterval = val;
            needSave = true;
          }
        }
        if (doc.containsKey("forcedWateringDuration")) {
          uint32_t val = doc["forcedWateringDuration"];
          if (sett.forcedWateringDuration != val) {
            sett.forcedWateringDuration = val;
            needSave = true;
          }
        }
        if (doc.containsKey("morningWateringCount")) {
          uint8_t val = doc["morningWateringCount"];
          if (sett.morningWateringCount != val) {
            sett.morningWateringCount = val;
            needSave = true;
          }
        }
        if (doc.containsKey("morningWateringInterval")) {
          uint64_t tempVal = (uint64_t)doc["morningWateringInterval"];
          uint32_t newVal = (uint32_t)tempVal;
          if (sett.morningWateringInterval != newVal) {
            sett.morningWateringInterval = newVal;
            needSave = true;
          }
        }
        if (doc.containsKey("morningWateringDuration")) {
          uint64_t tempVal = (uint64_t)doc["morningWateringDuration"];
          uint32_t newVal = (uint32_t)tempVal;
          if (sett.morningWateringDuration != newVal) {
            sett.morningWateringDuration = newVal;
            needSave = true;
          }
        }
        if (doc.containsKey("wateringMode")) {
          int val = doc["wateringMode"];
          if (sett.wateringMode != val) {
            if (val == 0 || val == 1) { // Явный переход в авто или ручной
              int prev = sett.wateringMode;
              if (prev == 2) { // Из forced
                resetForcedToNormal(val);
                needSave = true;
              } else {
                sett.wateringMode = val;
                needSave = true;
              }
              Serial.printf("DEBUG: Updated wateringMode to %d (explicit transition)\n", val);
            } else if (val == 2) {
              sett.wateringMode = 2; // Forced, но без активации
              needSave = true;
            }
          }
        }
        if (needSave) {
          saveSettings(saveCaller);
          Serial.println("Settings updated and saved from /settings");
          publishSettings();
          publishAllSensors(); // Явная публикация после HTTP изменений
        } else {
          Serial.println("No settings changed in /settings");
        }
        request->send(200, "application/json", "{\"status\":\"ok\"}");
        bufferIndex = 0;
      } else {
        Serial.println("Accumulating data...");
      }
    });
  server.on(
    "/cmd/watering", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (index == 0 && total > 0 && len == total) {
        StaticJsonDocument<256> jsonDoc;
        DeserializationError error = deserializeJson(jsonDoc, data, len);
        if (error) {
          Serial.printf("JSON parsing error in /cmd/watering: %s\n", error.c_str());
          request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
          return;
        }
        if (!jsonDoc.containsKey("command")) {
          Serial.println("Missing 'command' key in JSON body for /cmd/watering");
          request->send(400, "application/json", "{\"error\":\"Missing command parameter\"}");
          return;
        }
        String command = jsonDoc["command"].as<String>();
        Serial.printf("Received command for watering from JSON: %s\n", command.c_str());
        float currentLevel = 0;
        xQueuePeek(qLevel, &currentLevel, 0);
        DateTime nowDt = rtc.now();
        uint32_t nowUnix = nowDt.unixtime();
        if (command == "force-on") {
          if (sett.morningWateringActive) {
            sett.pendingMorningComplete = true;
            LOG("DEBUG MORNING-WATER: Force-on during morning — pending complete set\n");
          }
          sett.prevWateringMode = sett.wateringMode;
          sett.forceWateringActive = true;
          uint32_t durationMs = sett.forcedWateringDuration;
          if (durationMs == 0 || durationMs > 3600000UL) durationMs = 30000UL;
          sett.forceWateringEndTimeUnix = nowUnix + (durationMs / 1000UL);
          sett.wateringMode = 2;
          sett.pumpState = true;
          if (sett.forceWateringOverride) sett.forceWateringOverride = false;
          Serial.printf("HTTP Force watering ON for %u ms (unix %lu)\n", durationMs, nowUnix);
          saveSettings("cmd_watering_force_on");
          publishSettings();
          publishAllSensors(); // Явная публикация
          request->send(200, "application/json", "{\"status\":\"ok\", \"message\":\"Принудительный полив включен\"}");
        } else if (command == "force-off") {
          sett.forceWateringActive = false;
          sett.pumpState = false;
          sett.fillState = false;
          sett.hydroMix = false;
          sett.hydroStartUnix = 0;
          sett.wateringMode = 2;
          sett.forceWateringOverride = true;
          sett.forceWateringEndTimeUnix = 0;
          Serial.println("HTTP Force watering OFF (DISABLED, flags set)");
          saveSettings("cmd_watering_force_off");
          publishSettings();
          publishAllSensors(); // Явная публикация
          request->send(200, "application/json", "{\"status\":\"ok\", \"message\":\"Принудительное отключение полива\"}");
        } else if (command == "set-auto") {
          resetForcedToNormal(0);
          saveSettings("cmd_watering_set_auto");
          publishSettings();
          publishAllSensors(); // Явная публикация
          request->send(200, "application/json", "{\"status\":\"ok\", \"message\":\"Переход в автоматический режим\"}");
        } else if (command == "set-manual") {
          resetForcedToNormal(1);
          saveSettings("cmd_watering_set_manual");
          publishSettings();
          publishAllSensors(); // Явная публикация
          request->send(200, "application/json", "{\"status\":\"ok\", \"message\":\"Переход в ручной режим\"}");
        } else if (command == "fill") {
          if (currentLevel >= 99.0) {
            Serial.println("HTTP Fill water failed: Tank almost full");
            request->send(400, "application/json", "{\"error\":\"Бочка уже почти полная\"}");
            return;
          }
          if (sett.morningWateringActive) {
            sett.pendingMorningComplete = true;
            LOG("DEBUG MORNING-WATER: Fill during morning — pending complete set\n");
          }
          sett.prevWateringMode = sett.wateringMode;
          sett.forceWateringActive = true;
          sett.fillState = true;
          sett.wateringMode = 2;
          if (sett.forceWateringOverride) sett.forceWateringOverride = false;
          sett.forceWateringEndTimeUnix = nowUnix + 7200UL;
          Serial.printf("HTTP Fill water started (flag set for LogicTask) (unix %lu)\n", nowUnix);
          saveSettings("cmd_watering_fill");
          publishSettings();
          publishAllSensors(); // Явная публикация
          request->send(200, "application/json", "{\"status\":\"ok\", \"message\":\"Набор воды начат\"}");
        } else if (command == "hydro-mix") {
          if (sett.morningWateringActive) {
            sett.pendingMorningComplete = true;
            LOG("DEBUG MORNING-WATER: Hydro-mix during morning — pending complete set\n");
          }
          sett.prevWateringMode = sett.wateringMode;
          sett.forceWateringActive = true;
          uint32_t duration = sett.hydroMixDuration;
          if (duration == 0) duration = 10 * 60 * 1000UL;
          sett.forceWateringEndTimeUnix = nowUnix + (duration / 1000UL);
          sett.hydroMix = true;
          sett.hydroStartUnix = nowUnix;
          sett.pumpState = true;
          sett.wateringMode = 2;
          if (sett.forceWateringOverride) sett.forceWateringOverride = false;
          Serial.printf("HTTP Hydro mix started for %u ms (unix %lu)\n", duration, nowUnix);
          saveSettings("cmd_watering_hydro_mix");
          publishSettings();
          publishAllSensors(); // Явная публикация
          request->send(200, "application/json", "{\"status\":\"ok\", \"message\":\"Гидроразмешивание начато\"}");
        } else {
          Serial.printf("Unknown watering command: %s\n", command.c_str());
          request->send(400, "application/json", "{\"error\":\"Unknown watering command\"}");
        }
      } else {
        Serial.println("Invalid or incomplete data for /cmd/watering");
        request->send(400, "application/json", "{\"error\":\"Invalid request data\"}");
      }
    });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    SensorData avg;
    float sol, level, wind;
    SensorData outdoor;
    float pyrano;
    MatSensorData mat;
    xQueuePeek(qAvg, &avg, 0);
    xQueuePeek(qSol, &sol, 0);
    xQueuePeek(qLevel, &level, 0);
    xQueuePeek(qWind, &wind, 0);
    xQueuePeek(qOutdoor, &outdoor, 0);
    xQueuePeek(qPyrano, &pyrano, 0);
    xQueuePeek(qMat, &mat, 0);
    DateTime now = rtc.now();
    doc["timestamp"] = now.unixtime() - 10800;
    doc["wifi"] = WiFi.status() == WL_CONNECTED;
    doc["avgTemp"] = isnan(avg.temperature) ? 0 : avg.temperature;
    doc["avgHum"] = isnan(avg.humidity) ? 0 : avg.humidity;
    doc["solutionTemp"] = isnan(sol) ? 0 : sol;
    doc["waterLevel"] = isnan(level) ? 0 : level;
    doc["windSpeed"] = isnan(wind) ? 0 : wind;
    doc["windowPos"] = windowPos;
    doc["isHeatingOn"] = digitalRead(REL_HEAT);
    doc["isFanOn"] = digitalRead(REL_FAN);
    doc["isFogOn"] = digitalRead(REL_FOG_PUMP);
    doc["isWateringOn"] = digitalRead(REL_PUMP);
    doc["isSolutionHeatingOn"] = digitalRead(REL_SOL);
    doc["hydroMix"] = sett.hydroMix;
    doc["outdoorTemp"] = isnan(outdoor.temperature) ? 0 : outdoor.temperature;
    doc["outdoorHum"] = isnan(outdoor.humidity) ? 0 : outdoor.humidity;
    doc["pyrano"] = isnan(pyrano) ? 0 : pyrano;
    doc["matTemp"] = isnan(mat.temperature) ? 0 : mat.temperature;
    doc["matHum"] = isnan(mat.humidity) ? 0 : mat.humidity;
    doc["matEC"] = isnan(mat.ec) ? 0 : mat.ec;
    doc["matPH"] = isnan(mat.ph) ? 0 : mat.ph;
    doc["radSum"] = sett.radSum;
    doc["cycleCount"] = sett.cycleCount;
    doc["forceWateringActive"] = sett.forceWateringActive;
    doc["forceWateringOverride"] = sett.forceWateringOverride;
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });
  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(4096);
    JsonArray arr = doc.createNestedArray("history");
    for (int i = 0; i < HISTORY_SIZE; i++) {
      int idx = (historyIndex + i) % HISTORY_SIZE;
      if (history[idx].timestamp == 0) continue;
      JsonObject obj = arr.createNestedObject();
      obj["timestamp"] = history[idx].timestamp;
      obj["avgTemp"] = history[idx].avgTemp;
      obj["avgHum"] = history[idx].avgHum;
      obj["level"] = history[idx].level;
      obj["windSpeed"] = history[idx].windSpeed;
      obj["outdoorTemp"] = history[idx].outdoorTemp;
      obj["outdoorHum"] = history[idx].outdoorHum;
      obj["windowPosition"] = history[idx].windowPosition;
      obj["fanState"] = history[idx].fanState;
      obj["heatingState"] = history[idx].heatingState;
      obj["wateringState"] = history[idx].wateringState;
      obj["solutionHeatingState"] = history[idx].solutionHeatingState;
      obj["hydroState"] = history[idx].hydroState;
      obj["fillState"] = history[idx].fillState;
      obj["fogState"] = history[idx].fogState;
    }
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });
  server.on(
    "/cmd/window", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      static String bodyBuffer;
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((const char *)data, len);
      if (len + index == total) {
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, bodyBuffer);
        if (error) {
          Serial.println("Bad cmd/window JSON");
          request->send(400, "application/json", "{\"error\":\"Bad JSON\"}");
          return;
        }
        if (!doc.containsKey("direction")) {
          Serial.println("Missing direction in cmd/window");
          request->send(400, "application/json", "{\"error\":\"Missing direction\"}");
          return;
        }
        String dir = doc["direction"].as<String>();
        uint32_t dur = doc.containsKey("duration") ? doc["duration"].as<uint32_t>() : 0;
        if (dir == "up" || dir == "down") {
          startWindowMovement(dir, dur);
          sett.manualWindow = true;
          saveSettings("http_window_cmd");
        } else if (dir == "stop") {
          updateWindowPosition();
        } else {
          Serial.println("Invalid direction");
          request->send(400, "application/json", "{\"error\":\"Invalid direction\"}");
          return;
        }
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      }
    });
  server.on(
    "/cmd/equipment", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
      Serial.println("Received /cmd/equipment POST request");
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, (const char *)data, len);
      if (error) {
        Serial.printf("JSON parsing error: %s\n", error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      if (!doc.containsKey("equipment") || !doc.containsKey("state")) {
        Serial.println("Missing equipment or state parameters");
        request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
        return;
      }
      String equipment = doc["equipment"].as<String>();
      bool state = doc["state"].as<bool>();
      Serial.printf("Equipment: %s, State: %d\n", equipment.c_str(), state);
      bool needSave = false;
      if (equipment == "fan") {
        sett.manualFan = true;
        sett.fanState = state;
        needSave = true;
        Serial.printf("Fan set to %s\n", state ? "ON" : "OFF");
      } else if (equipment == "heating") {
        sett.manualHeat = true;
        sett.heatState = state;
        needSave = true;
        Serial.printf("Heating set to %s\n", state ? "ON" : "OFF");
      } else if (equipment == "watering") {
        sett.manualPump = true;
        sett.manualPumpOverride = doc.containsKey("override") ? doc["override"].as<bool>() : false;
        sett.pumpState = state;
        needSave = true;
        Serial.printf("Watering set to %s, Override: %d\n", state ? "ON" : "OFF", sett.manualPumpOverride);
      } else if (equipment == "solution-heating") {
        sett.manualSol = true;
        sett.solHeatState = state;
        needSave = true;
        Serial.printf("Solution heating set to %s\n", state ? "ON" : "OFF");
      } else if (equipment == "fog-mode-auto") {
        sett.fogMode = 0;
        sett.forceFogOn = false;
        sett.fogState = false;
        needSave = true;
        Serial.println("Fog mode set to Auto");
      } else if (equipment == "fog-mode-manual") {
        sett.fogMode = 1;
        sett.forceFogOn = false;
        sett.fogState = false;
        needSave = true;
        Serial.println("Fog mode set to Manual");
      } else if (equipment == "fog-mode-forced") {
        sett.fogMode = 2;
        sett.forceFogOn = state;
        needSave = true;
      } else {
        Serial.printf("Invalid equipment: %s\n", equipment.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid equipment\"}");
        return;
      }
      if (needSave) {
        saveSettings("cmd_equipment");
        publishSettings();
        publishAllSensors(); // Явная публикация после HTTP
      }
      Serial.println("Equipment command executed successfully");
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  publishSettings();
  Serial.println(WiFi.localIP());
  server.begin();
}
void loop() {}