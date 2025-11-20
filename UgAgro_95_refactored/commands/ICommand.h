#ifndef ICOMMAND_H
#define ICOMMAND_H

#include <ArduinoJson.h>

/**
 * @brief Базовый интерфейс для всех команд
 *
 * Паттерн Command позволяет инкапсулировать операции над настройками
 * в отдельные объекты, устраняя дублирование кода между MQTT и Web handlers
 */
class ICommand {
public:
  virtual ~ICommand() = default;

  /**
   * @brief Выполнить команду
   * @param doc JSON документ с параметрами
   * @param needSave Флаг необходимости сохранения настроек
   * @return true если команда выполнена успешно
   */
  virtual bool execute(JsonDocument& doc, bool& needSave) = 0;

  /**
   * @brief Получить имя команды
   * @return Имя команды (ключ в JSON)
   */
  virtual const char* getName() const = 0;

  /**
   * @brief Валидация значения перед применением
   * @param doc JSON документ с параметрами
   * @return true если значение валидно
   */
  virtual bool validate(JsonDocument& doc) const { return true; }
};

#endif // ICOMMAND_H
