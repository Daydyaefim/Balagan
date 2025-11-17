#ifndef SETTING_COMMANDS_H
#define SETTING_COMMANDS_H

#include "ICommand.h"
#include <functional>

/**
 * @brief Базовый шаблонный класс для команд изменения настроек
 *
 * @tparam T Тип значения настройки (float, bool, uint8_t, uint32_t, и т.д.)
 */
template<typename T>
class SettingCommand : public ICommand {
protected:
  const char* name;
  T& settingRef;
  std::function<bool(T)> validator;

public:
  /**
   * @brief Конструктор команды
   * @param cmdName Имя команды (ключ в JSON)
   * @param setting Ссылка на переменную настройки
   * @param validatorFunc Функция валидации (опционально)
   */
  SettingCommand(const char* cmdName, T& setting,
                 std::function<bool(T)> validatorFunc = nullptr)
    : name(cmdName), settingRef(setting), validator(validatorFunc) {}

  const char* getName() const override {
    return name;
  }

  bool validate(JsonDocument& doc) const override {
    if (!doc.containsKey(name)) {
      return false;
    }
    if (validator) {
      T value = doc[name].as<T>();
      return validator(value);
    }
    return true;
  }

  bool execute(JsonDocument& doc, bool& needSave) override {
    if (!doc.containsKey(name)) {
      return false;
    }

    T newValue = doc[name].as<T>();
    if (settingRef != newValue) {
      settingRef = newValue;
      needSave = true;
      return true;
    }
    return false;
  }
};

/**
 * @brief Специализация для float с диапазоном
 */
class FloatRangeCommand : public SettingCommand<float> {
private:
  float minValue;
  float maxValue;

public:
  FloatRangeCommand(const char* cmdName, float& setting, float min, float max)
    : SettingCommand<float>(cmdName, setting, nullptr),
      minValue(min), maxValue(max) {}

  bool validate(JsonDocument& doc) const override {
    if (!doc.containsKey(name)) {
      return false;
    }
    float value = doc[name].as<float>();
    return value >= minValue && value <= maxValue;
  }
};

/**
 * @brief Специализация для uint8_t с диапазоном (0-255)
 */
class UInt8RangeCommand : public SettingCommand<uint8_t> {
private:
  uint8_t minValue;
  uint8_t maxValue;

public:
  UInt8RangeCommand(const char* cmdName, uint8_t& setting,
                    uint8_t min = 0, uint8_t max = 255)
    : SettingCommand<uint8_t>(cmdName, setting, nullptr),
      minValue(min), maxValue(max) {}

  bool validate(JsonDocument& doc) const override {
    if (!doc.containsKey(name)) {
      return false;
    }
    int value = doc[name].as<int>();
    return value >= minValue && value <= maxValue;
  }
};

/**
 * @brief Специализация для uint32_t
 */
class UInt32Command : public SettingCommand<uint32_t> {
private:
  uint32_t minValue;
  uint32_t maxValue;

public:
  UInt32Command(const char* cmdName, uint32_t& setting,
                uint32_t min = 0, uint32_t max = UINT32_MAX)
    : SettingCommand<uint32_t>(cmdName, setting, nullptr),
      minValue(min), maxValue(max) {}

  bool validate(JsonDocument& doc) const override {
    if (!doc.containsKey(name)) {
      return false;
    }
    uint32_t value = doc[name].as<uint32_t>();
    return value >= minValue && value <= maxValue;
  }
};

/**
 * @brief Команда для bool без дополнительной валидации
 */
class BoolCommand : public SettingCommand<bool> {
public:
  BoolCommand(const char* cmdName, bool& setting)
    : SettingCommand<bool>(cmdName, setting, nullptr) {}
};

/**
 * @brief Команда для int с диапазоном
 */
class IntRangeCommand : public SettingCommand<int> {
private:
  int minValue;
  int maxValue;

public:
  IntRangeCommand(const char* cmdName, int& setting, int min, int max)
    : SettingCommand<int>(cmdName, setting, nullptr),
      minValue(min), maxValue(max) {}

  bool validate(JsonDocument& doc) const override {
    if (!doc.containsKey(name)) {
      return false;
    }
    int value = doc[name].as<int>();
    return value >= minValue && value <= maxValue;
  }
};

#endif // SETTING_COMMANDS_H
