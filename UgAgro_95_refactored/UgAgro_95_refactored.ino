// ===========================
// УМНАЯ ТЕПЛИЦА – РЕФАКТОРИРОВАННАЯ ВЕРСИЯ
// Версия: 2.0 (Модульная архитектура)
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
#include <Wire.h>
#include <RTClib.h>

// Подключение модулей проекта
#include "Config.h"
#include "Sensors.h"
#include "Actuators.h"
#include "MqttManager.h"
#include "Storage.h"
#include "ControlLogic.h"
#include "commands/CommandDispatcher.h"
#include "commands/AllCommands.h"

// ===========================
// ГЛОБАЛЬНЫЕ ОБЪЕКТЫ
// ===========================

// RTC
RTC_DS3231 rtc;

// Последовательный порт для Modbus
HardwareSerial RS485(1);
ModbusMaster mb;

// WiFi и MQTT
WiFiClient wifiClient;
WiFiUDP udp;
NTPClient timeClient(udp, NTP_SERVER, NTP_TIMEZONE_OFFSET);

// Веб-сервер
AsyncWebServer server(80);

// Очереди FreeRTOS для данных датчиков
QueueHandle_t qAvg, qSol, qLevel, qWind, qOutdoor, qPyrano, qMat;
SemaphoreHandle_t mtx;

// Менеджеры модулей
SensorManager* sensorManager = nullptr;
ActuatorManager* actuatorManager = nullptr;
MqttManager* mqttManager = nullptr;
StorageManager* storageManager = nullptr;
GreenhouseController* controller = nullptr;

// Command Pattern для обработки настроек
CommandDispatcher* commandDispatcher = nullptr;
AllCommands* allCommands = nullptr;

// Задачи FreeRTOS
TaskHandle_t networkTimeTaskHandle = NULL;
volatile bool forceWifiReconnect = false;
volatile bool wifiConnectedOnce = false;
unsigned long lastNetworkCheckTime = 0;
unsigned long lastTimeUpdateTime = 0;

// ===========================
// ПРОТОТИПЫ ФУНКЦИЙ
// ===========================

void setupRTC();
void setupWiFi();
void setupMQTT();
void setupWebServer();
void setupQueues();
void setupTasks();
void networkTimeManagementTask(void* parameter);

// ===========================
// SETUP
// ===========================

void setup() {
  Serial.begin(115200);
  Serial.println("=== УМНАЯ ТЕПЛИЦА - ЗАПУСК ===");

  // Инициализация I2C для RTC
  Wire.begin();
  setupRTC();

  // Инициализация файловой системы
  if (!LittleFS.begin()) {
    Serial.println("ОШИБКА: Не удалось монтировать LittleFS");
    return;
  }

  // Создание менеджеров
  storageManager = new StorageManager();
  storageManager->begin();

  actuatorManager = new ActuatorManager();
  actuatorManager->begin();
  actuatorManager->calibrateWindow(); // Калибровка окна при старте

  // Инициализация WiFi
  setupWiFi();

  // Создание MQTT менеджера
  mqttManager = new MqttManager(wifiClient, rtc);
  mqttManager->begin();

  // Установка callback для сохранения настроек
  mqttManager->setSaveSettingsCallback([](const char* caller) {
    if (storageManager && controller) {
      storageManager->saveSettings(controller->getSettings(), caller);
    }
  });

  setupMQTT();

  // Создание менеджера датчиков
  sensorManager = new SensorManager(mb, RS485);
  sensorManager->begin();

  // Создание очередей
  setupQueues();

  // Установка очередей для менеджера датчиков
  sensorManager->setQueues(qAvg, qSol, qLevel, qWind, qPyrano, qMat, qOutdoor);

  // Создание контроллера теплицы
  mtx = xSemaphoreCreateMutex();
  controller = new GreenhouseController(*sensorManager, *actuatorManager,
                                         *mqttManager, *storageManager,
                                         rtc, mtx);
  controller->begin();
  controller->setQueues(qAvg, qSol, qLevel, qWind, qPyrano, qMat, qOutdoor);

  // Установка ссылки на настройки для MQTT
  mqttManager->setSettingsReference(&controller->getSettings());

  // Инициализация CommandDispatcher для веб-сервера и MQTT
  commandDispatcher = new CommandDispatcher(DEBUG);
  allCommands = new AllCommands(controller->getSettings(), *commandDispatcher);
  Serial.printf("CommandDispatcher инициализирован с %d командами\n", allCommands->getCommandCount());

  // Установка CommandDispatcher для MQTT
  mqttManager->setCommandDispatcher(commandDispatcher);

  // Настройка веб-сервера
  setupWebServer();
  server.begin();
  Serial.println("Веб-сервер запущен");

  // Создание задач FreeRTOS
  setupTasks();

  Serial.println("=== ИНИЦИАЛИЗАЦИЯ ЗАВЕРШЕНА ===");
}

// ===========================
// LOOP
// ===========================

void loop() {
  // Основная логика вынесена в FreeRTOS задачи
  // Здесь можно обрабатывать MQTT
  if (mqttManager) {
    mqttManager->loop();
  }

  delay(10);
}

// ===========================
// ФУНКЦИИ ИНИЦИАЛИЗАЦИИ
// ===========================

void setupRTC() {
  if (!rtc.begin()) {
    Serial.println("ОШИБКА: RTC DS3231 не найден!");
    delay(2000);
    return;
  }

  if (rtc.lostPower()) {
    Serial.println("RTC потерял питание, установка времени из компиляции");
    // Простая установка времени (можно улучшить)
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  DateTime now = rtc.now();
  Serial.printf("Текущее время RTC: %02d:%02d:%02d %02d/%02d/%04d (epoch: %lu)\n",
                now.hour(), now.minute(), now.second(),
                now.day(), now.month(), now.year(), now.unixtime());
}

void setupWiFi() {
  Serial.printf("Подключение к WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi подключен!");
    Serial.print("IP адрес: ");
    Serial.println(WiFi.localIP());
    wifiConnectedOnce = true;

    // Обновление RTC через NTP
    timeClient.begin();
    timeClient.update();
    delay(2000);

    unsigned long syncStart = millis();
    while ((rtc.now().unixtime() <= 1704067200UL) && (millis() - syncStart < 30000)) {
      if (timeClient.update()) {
        unsigned long epoch = timeClient.getEpochTime();
        if (epoch > 1704067200UL) {
          rtc.adjust(DateTime(epoch));
          Serial.println("RTC обновлен через NTP");
          break;
        }
      }
      Serial.print("Попытка NTP... ");
      delay(2000);
    }
  } else {
    Serial.println("\nНе удалось подключиться к WiFi");
  }
}

void setupMQTT() {
  if (mqttManager && WiFi.status() == WL_CONNECTED) {
    if (mqttManager->connect()) {
      if (controller) {
        mqttManager->publishSettings(controller->getSettings());
      }
    }
  }
}

void setupQueues() {
  qAvg = xQueueCreate(1, sizeof(SensorData));
  qSol = xQueueCreate(1, sizeof(float));
  qLevel = xQueueCreate(1, sizeof(float));
  qWind = xQueueCreate(1, sizeof(float));
  qOutdoor = xQueueCreate(1, sizeof(SensorData));
  qPyrano = xQueueCreate(1, sizeof(float));
  qMat = xQueueCreate(1, sizeof(MatSensorData));

  Serial.println("Очереди FreeRTOS созданы");
}

void setupTasks() {
  // Задача чтения датчиков Modbus
  xTaskCreate(modbusTask, "modbus", 4096, sensorManager, 1, NULL);

  // Задача логики управления
  xTaskCreatePinnedToCore(logicTask, "logic", 12288, controller, 1, NULL, 0);

  // Задача управления сетью и временем
  xTaskCreate(networkTimeManagementTask, "Network & Time Mgmt", 4096, NULL, 1, &networkTimeTaskHandle);

  Serial.println("Задачи FreeRTOS созданы");
}

// ===========================
// ВЕБЕРСЕРВЕР
// ===========================

void setupWebServer() {
  // Главная страница
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  // API для получения настроек
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!controller) {
      request->send(500, "text/plain", "Controller not initialized");
      return;
    }

    DynamicJsonDocument doc(4096);
    Settings& sett = controller->getSettings();

    // Базовые настройки
    doc["minTemp"] = sett.minTemp;
    doc["maxTemp"] = sett.maxTemp;
    doc["minHum"] = sett.minHum;
    doc["maxHum"] = sett.maxHum;
    doc["heatOn"] = sett.heatOn;
    doc["heatOff"] = sett.heatOff;
    // ... (добавить остальные поля по аналогии)

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // API для обновления настроек
  server.on("/settings", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (!controller || !commandDispatcher) {
        request->send(500, "text/plain", "Controller not initialized");
        return;
      }

      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "text/plain", "Invalid JSON");
        return;
      }

      // Используем CommandDispatcher для обработки всех полей
      bool needSave = false;
      int executedCount = commandDispatcher->processDocument(doc, needSave);

      Serial.printf("WebServer: Обработано %d команд из POST /settings\n", executedCount);

      if (needSave && storageManager) {
        storageManager->saveSettings(controller->getSettings(), "web_api");
      }

      // Создаем ответ
      DynamicJsonDocument resp(256);
      resp["success"] = true;
      resp["commands_executed"] = executedCount;
      String json;
      serializeJson(resp, json);

      request->send(200, "application/json", json);
    }
  );

  // API для получения данных датчиков
  server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(2048);

    // Чтение данных из очередей
    SensorData avgData;
    float sol = 0, level = 0, wind = 0, pyrano = 0;

    if (xQueuePeek(qAvg, &avgData, 0) == pdTRUE) {
      doc["temperature"] = avgData.temperature;
      doc["humidity"] = avgData.humidity;
    }

    xQueuePeek(qSol, &sol, 0);
    xQueuePeek(qLevel, &level, 0);
    xQueuePeek(qWind, &wind, 0);
    xQueuePeek(qPyrano, &pyrano, 0);

    doc["solution_temperature"] = sol;
    doc["water_level"] = level;
    doc["wind_speed"] = wind;
    doc["pyrano"] = pyrano;

    if (actuatorManager) {
      doc["window_position"] = actuatorManager->getWindowPosition();
    }

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // API для управления окном
  server.on("/window", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (!controller) {
        request->send(500, "text/plain", "Controller not initialized");
        return;
      }

      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, data, len);

      if (error || !doc.containsKey("direction")) {
        request->send(400, "text/plain", "Invalid JSON or missing direction");
        return;
      }

      String dir = doc["direction"].as<String>();
      uint32_t dur = doc.containsKey("duration") ? doc["duration"].as<uint32_t>() : 0;

      Settings& sett = controller->getSettings();
      sett.manualWindow = true;
      sett.windowCommand = dir;
      sett.windowDuration = dur;

      request->send(200, "text/plain", "Window command sent");
    }
  );

  // API для управления оборудованием
  server.on("/equipment", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (!controller) {
        request->send(500, "text/plain", "Controller not initialized");
        return;
      }

      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, data, len);

      if (error || !doc.containsKey("equipment") || !doc.containsKey("state")) {
        request->send(400, "text/plain", "Invalid JSON");
        return;
      }

      String eq = doc["equipment"].as<String>();
      bool state = doc["state"].as<bool>();
      Settings& sett = controller->getSettings();
      bool needSave = false;

      if (eq == "fan") {
        sett.manualFan = true;
        sett.fanState = state;
        needSave = true;
      } else if (eq == "heating") {
        sett.manualHeat = true;
        sett.heatState = state;
        needSave = true;
      } else if (eq == "watering") {
        sett.manualPump = true;
        sett.pumpState = state;
        needSave = true;
      }

      if (needSave && storageManager) {
        storageManager->saveSettings(sett, "web_equipment");
      }

      request->send(200, "text/plain", "Equipment updated");
    }
  );
}

// ===========================
// ЗАДАЧА УПРАВЛЕНИЯ СЕТЬЮ И ВРЕМЕНЕМ
// ===========================

void networkTimeManagementTask(void* parameter) {
  for (;;) {
    unsigned long now = millis();

    // Проверка WiFi подключения
    if (WiFi.status() != WL_CONNECTED || forceWifiReconnect) {
      if (now - lastNetworkCheckTime > NETWORK_CHECK_INTERVAL) {
        Serial.println("Попытка переподключения к WiFi...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        unsigned long connectStart = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - connectStart < 10000) {
          vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("WiFi переподключен");
          wifiConnectedOnce = true;
          forceWifiReconnect = false;

          // Переподключение к MQTT
          if (mqttManager) {
            mqttManager->connect();
          }
        }

        lastNetworkCheckTime = now;
      }
    }

    // Обновление времени через NTP
    if (WiFi.status() == WL_CONNECTED && now - lastTimeUpdateTime > TIME_UPDATE_INTERVAL) {
      if (timeClient.update()) {
        unsigned long epoch = timeClient.getEpochTime();
        if (epoch > 1704067200UL) {
          rtc.adjust(DateTime(epoch));
          Serial.println("RTC обновлен через NTP");
        }
      }
      lastTimeUpdateTime = now;
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}
