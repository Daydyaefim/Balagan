// ===========================
// МОДУЛЬ ХРАНЕНИЯ ДАННЫХ
// ===========================

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Config.h"

// Класс для управления хранением данных в файловой системе
class StorageManager {
public:
  // Конструктор
  StorageManager();

  // Инициализация файловой системы
  bool begin();

  // Загрузка настроек из файла
  bool loadSettings(Settings& settings);

  // Сохранение настроек в файл
  bool saveSettings(const Settings& settings, const char* caller = "unknown");

  // Загрузка истории из файла
  bool loadHistory(HistoryEntry* history, int& count, int maxSize);

  // Сохранение истории в файл
  bool saveHistory(const HistoryEntry* history, int count);

  // Применение настроек по умолчанию
  void applyDefaultSettings(Settings& settings);

  // Проверка, что настройки обнулены (некорректные)
  bool isSettingsZeroed(const Settings& settings);

  // Получить время последнего сохранения
  unsigned long getLastSaveTime() const { return _lastSaveTime; }

private:
  unsigned long _lastSaveTime;
  static const unsigned long SAVE_COOLDOWN = 5000; // 5 секунд между сохранениями

  // Константы путей к файлам
  static const char* SETTINGS_FILE;
  static const char* HISTORY_FILE;

  // Вспомогательные функции
  bool serializeSettings(const Settings& settings, DynamicJsonDocument& doc);
  bool deserializeSettings(Settings& settings, const DynamicJsonDocument& doc);
};

#endif // STORAGE_H
