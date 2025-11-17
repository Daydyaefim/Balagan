#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include "ICommand.h"
#include <map>
#include <Arduino.h>

/**
 * @brief Диспетчер команд для централизованной обработки
 *
 * Используется как в MQTT callback, так и в Web handlers,
 * устраняя дублирование кода
 */
class CommandDispatcher {
private:
  std::map<String, ICommand*> commands;
  bool debugMode;

public:
  CommandDispatcher(bool debug = false) : debugMode(debug) {}

  /**
   * @brief Зарегистрировать команду
   * @param cmd Указатель на команду
   */
  void registerCommand(ICommand* cmd) {
    if (cmd) {
      commands[cmd->getName()] = cmd;
      if (debugMode) {
        Serial.printf("Registered command: %s\n", cmd->getName());
      }
    }
  }

  /**
   * @brief Выполнить команду по имени
   * @param name Имя команды
   * @param doc JSON документ с параметрами
   * @param needSave Флаг необходимости сохранения
   * @return true если команда найдена и выполнена
   */
  bool dispatch(const char* name, JsonDocument& doc, bool& needSave) {
    auto it = commands.find(name);
    if (it != commands.end()) {
      if (debugMode) {
        Serial.printf("Executing command: %s\n", name);
      }
      if (it->second->validate(doc)) {
        return it->second->execute(doc, needSave);
      } else {
        if (debugMode) {
          Serial.printf("Validation failed for command: %s\n", name);
        }
        return false;
      }
    }
    return false;
  }

  /**
   * @brief Обработать весь JSON документ
   * @param doc JSON документ с несколькими командами
   * @param needSave Флаг необходимости сохранения
   * @return Количество выполненных команд
   */
  int processDocument(JsonDocument& doc, bool& needSave) {
    int executed = 0;
    for (JsonPair kv : doc.as<JsonObject>()) {
      const char* key = kv.key().c_str();
      if (dispatch(key, doc, needSave)) {
        executed++;
      }
    }
    return executed;
  }

  /**
   * @brief Проверить, зарегистрирована ли команда
   */
  bool hasCommand(const char* name) const {
    return commands.find(name) != commands.end();
  }

  /**
   * @brief Получить количество зарегистрированных команд
   */
  size_t getCommandCount() const {
    return commands.size();
  }

  /**
   * @brief Очистить все зарегистрированные команды
   */
  void clear() {
    commands.clear();
  }
};

#endif // COMMAND_DISPATCHER_H
