// ===========================
// МОДУЛЬ УПРАВЛЕНИЯ MQTT
// ===========================

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <RTClib.h>
#include "Config.h"
#include "commands/CommandDispatcher.h"
#include "commands/AllCommands.h"

// Прототипы функций обратного вызова
typedef void (*SettingsUpdateCallback)(Settings& settings, bool needSave);
typedef void (*CommandCallback)(const char* command, const char* value);
typedef void (*SaveSettingsCallback)(const char* caller);

// Класс для управления MQTT подключением и публикациями
class MqttManager {
public:
  // Конструктор
  MqttManager(WiFiClient& wifiClient, RTC_DS3231& rtc);

  // Инициализация
  void begin(const char* clientId = "ESP32_Greenhouse");

  // Основной цикл обработки (нужно вызывать регулярно)
  void loop();

  // Подключение к MQTT брокеру
  bool connect();

  // Проверка подключения
  bool isConnected() const;

  // Публикация данных датчиков
  void publishAllSensors(const SensorData& avgData, float sol, float level, float wind,
                         const SensorData& outdoor, const MatSensorData& mat, float pyrano,
                         int windowPos, const Settings& settings);

  // Публикация настроек
  void publishSettings(const Settings& settings);

  // Публикация истории
  void publishHistory(const HistoryEntry* history, int size);

  // Публикация отдельных значений
  void publishWindowPosition(int position);
  void publishStatus(const char* status);

  // Установка callback для обработки входящих команд
  void setSettingsUpdateCallback(SettingsUpdateCallback callback);
  void setCommandCallback(CommandCallback callback);
  void setSaveSettingsCallback(SaveSettingsCallback callback);

  // Установка ссылки на настройки (для прямой модификации)
  void setSettingsReference(Settings* settings);

  // Установка внешнего CommandDispatcher
  void setCommandDispatcher(CommandDispatcher* dispatcher);

  // Подписка на топики
  void subscribe();

  // Получить клиент PubSub для прямого доступа (если нужно)
  PubSubClient& getClient() { return _client; }

private:
  PubSubClient _client;
  RTC_DS3231& _rtc;
  String _clientId;

  // Callback'и
  SettingsUpdateCallback _settingsUpdateCallback;
  CommandCallback _commandCallback;
  SaveSettingsCallback _saveSettingsCallback;

  // Ссылка на настройки (для прямой модификации)
  Settings* _settings;

  // Command Pattern для обработки настроек
  CommandDispatcher* _dispatcher;
  AllCommands* _allCommands;

  // Статическая функция callback для PubSubClient
  static void mqttCallback(char* topic, byte* payload, unsigned int length);

  // Глобальный указатель на текущий экземпляр (для статического callback)
  static MqttManager* _instance;

  // Внутренняя обработка сообщений
  void handleMessage(char* topic, byte* payload, unsigned int length);
  void handleSetSettings(const String& json);
  void handleEquipment(const String& json);
  void handleWindow(const String& json);
  void handleHydro();
  void handleCommand(const char* topic, const String& value);

  // Вспомогательные функции
  String createSensorJson(const SensorData& avgData, float sol, float level, float wind,
                          const SensorData& outdoor, const MatSensorData& mat, float pyrano,
                          int windowPos, const Settings& settings);
  String createSettingsJson(const Settings& settings);
};

#endif // MQTT_MANAGER_H
