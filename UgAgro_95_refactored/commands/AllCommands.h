#ifndef ALL_COMMANDS_H
#define ALL_COMMANDS_H

#include "CommandDispatcher.h"
#include "SettingCommands.h"
#include "../Config.h"
#include <vector>

/**
 * @brief Фабрика для создания и регистрации всех команд
 *
 * Этот класс управляет жизненным циклом всех команд и
 * автоматически регистрирует их в диспетчере
 */
class AllCommands {
private:
  std::vector<ICommand*> commands;
  Settings& settings;

  /**
   * @brief Добавить команду в список и зарегистрировать в диспетчере
   */
  void addCommand(ICommand* cmd, CommandDispatcher& dispatcher) {
    commands.push_back(cmd);
    dispatcher.registerCommand(cmd);
  }

public:
  /**
   * @brief Конструктор - создает все команды и регистрирует их
   * @param sett Ссылка на глобальные настройки
   * @param dispatcher Диспетчер команд
   */
  AllCommands(Settings& sett, CommandDispatcher& dispatcher) : settings(sett) {
    registerAllCommands(dispatcher);
  }

  /**
   * @brief Деструктор - освобождает память
   */
  ~AllCommands() {
    for (auto cmd : commands) {
      delete cmd;
    }
    commands.clear();
  }

  /**
   * @brief Регистрация всех команд (100+ полей Settings)
   */
  void registerAllCommands(CommandDispatcher& dispatcher) {
    // ============================================================
    // КЛИМАТ (4 команды)
    // ============================================================
    addCommand(new FloatRangeCommand("minTemp", settings.minTemp, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("maxTemp", settings.maxTemp, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("minHum", settings.minHum, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("maxHum", settings.maxHum, 0.0, 100.0), dispatcher);

    // ============================================================
    // ОКНА (5 команд)
    // ============================================================
    addCommand(new UInt32Command("openTime", settings.openTime, 1000, 300000), dispatcher);
    addCommand(new UInt32Command("closeTime", settings.closeTime, 1000, 300000), dispatcher);
    addCommand(new BoolCommand("manualWindow", settings.manualWindow), dispatcher);
    addCommand(new StringCommand("windowCommand", settings.windowCommand), dispatcher);
    addCommand(new UInt32Command("windowDuration", settings.windowDuration, 0, 300000), dispatcher);

    // ============================================================
    // ОТОПЛЕНИЕ (4 команды)
    // ============================================================
    addCommand(new FloatRangeCommand("heatOn", settings.heatOn, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("heatOff", settings.heatOff, -20.0, 50.0), dispatcher);
    addCommand(new BoolCommand("manualHeat", settings.manualHeat), dispatcher);
    addCommand(new BoolCommand("heatState", settings.heatState), dispatcher);

    // ============================================================
    // ВЕНТИЛЯТОР (4 команды)
    // ============================================================
    addCommand(new UInt32Command("fanInterval", settings.fanInterval, 60000, 86400000), dispatcher);
    addCommand(new UInt32Command("fanDuration", settings.fanDuration, 1000, 3600000), dispatcher);
    addCommand(new BoolCommand("manualFan", settings.manualFan), dispatcher);
    addCommand(new BoolCommand("fanState", settings.fanState), dispatcher);

    // ============================================================
    // ТУМАН (22 команды)
    // ============================================================
    addCommand(new UInt8RangeCommand("fogMorningStart", settings.fogMorningStart, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("fogMorningEnd", settings.fogMorningEnd, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("fogDayStart", settings.fogDayStart, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("fogDayEnd", settings.fogDayEnd, 0, 23), dispatcher);

    addCommand(new UInt32Command("fogMorningDuration", settings.fogMorningDuration, 1000, 60000), dispatcher);
    addCommand(new UInt32Command("fogMorningInterval", settings.fogMorningInterval, 10000, 3600000), dispatcher);
    addCommand(new UInt32Command("fogDayDuration", settings.fogDayDuration, 1000, 60000), dispatcher);
    addCommand(new UInt32Command("fogDayInterval", settings.fogDayInterval, 10000, 3600000), dispatcher);
    addCommand(new UInt32Command("fogDelay", settings.fogDelay, 0, 10000), dispatcher);

    addCommand(new FloatRangeCommand("fogMinHum", settings.fogMinHum, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("fogMaxHum", settings.fogMaxHum, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("fogMinTemp", settings.fogMinTemp, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("fogMaxTemp", settings.fogMaxTemp, -20.0, 50.0), dispatcher);

    addCommand(new UInt32Command("fogMinDuration", settings.fogMinDuration, 1000, 60000), dispatcher);
    addCommand(new UInt32Command("fogMaxDuration", settings.fogMaxDuration, 1000, 60000), dispatcher);
    addCommand(new UInt32Command("fogMinInterval", settings.fogMinInterval, 10000, 7200000), dispatcher);
    addCommand(new UInt32Command("fogMaxInterval", settings.fogMaxInterval, 10000, 7200000), dispatcher);

    addCommand(new IntRangeCommand("fogMode", settings.fogMode, 0, 2), dispatcher);
    addCommand(new BoolCommand("forceFogOn", settings.forceFogOn), dispatcher);
    addCommand(new UInt32Command("autoFogDayStart", settings.autoFogDayStart, 0, 86400), dispatcher);
    addCommand(new UInt32Command("autoFogDayEnd", settings.autoFogDayEnd, 0, 86400), dispatcher);
    addCommand(new BoolCommand("fogState", settings.fogState), dispatcher);

    // ============================================================
    // РАСТВОР (4 команды)
    // ============================================================
    addCommand(new FloatRangeCommand("solOn", settings.solOn, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("solOff", settings.solOff, -20.0, 50.0), dispatcher);
    addCommand(new BoolCommand("manualSol", settings.manualSol), dispatcher);
    addCommand(new BoolCommand("solHeatState", settings.solHeatState), dispatcher);

    // ============================================================
    // ВЕТЕР (3 команды)
    // ============================================================
    addCommand(new FloatRangeCommand("wind1", settings.wind1, 0.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("wind2", settings.wind2, 0.0, 50.0), dispatcher);
    addCommand(new UInt32Command("windLockMinutes", settings.windLockMinutes, 0, 1440), dispatcher);

    // ============================================================
    // ДЕНЬ/НОЧЬ (3 команды)
    // ============================================================
    addCommand(new UInt8RangeCommand("nightStart", settings.nightStart, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("dayStart", settings.dayStart, 0, 23), dispatcher);
    addCommand(new FloatRangeCommand("nightOffset", settings.nightOffset, -10.0, 10.0), dispatcher);

    // ============================================================
    // УРОВЕНЬ (3 команды)
    // ============================================================
    addCommand(new FloatRangeCommand("levelMin", settings.levelMin, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("levelMax", settings.levelMax, 0.0, 100.0), dispatcher);
    addCommand(new BoolCommand("fillState", settings.fillState), dispatcher);

    // ============================================================
    // ГИДРОПОНИКА (4 команды)
    // ============================================================
    addCommand(new UInt32Command("hydroMixDuration", settings.hydroMixDuration, 60000, 1800000), dispatcher);
    addCommand(new BoolCommand("hydroMix", settings.hydroMix), dispatcher);
    addCommand(new UInt32Command("hydroStart", settings.hydroStart), dispatcher);
    addCommand(new UInt32Command("hydroStartUnix", settings.hydroStartUnix), dispatcher);

    // ============================================================
    // ПОЛИВ - БАЗОВЫЕ (6 команд)
    // ============================================================
    addCommand(new BoolCommand("manualPump", settings.manualPump), dispatcher);
    addCommand(new BoolCommand("previousManualPump", settings.previousManualPump), dispatcher);
    addCommand(new BoolCommand("manualPumpOverride", settings.manualPumpOverride), dispatcher);
    addCommand(new BoolCommand("pumpState", settings.pumpState), dispatcher);
    addCommand(new IntRangeCommand("wateringMode", settings.wateringMode, -1, 3), dispatcher);
    addCommand(new UInt32Command("pumpTime", settings.pumpTime, 1000, 600000), dispatcher);

    // ============================================================
    // ПОЛИВ - FORCED (6 команд)
    // ============================================================
    addCommand(new BoolCommand("forceWateringActive", settings.forceWateringActive), dispatcher);
    addCommand(new BoolCommand("forceWateringOverride", settings.forceWateringOverride), dispatcher);
    addCommand(new UInt32Command("forceWateringEndTime", settings.forceWateringEndTime), dispatcher);
    addCommand(new UInt32Command("forceWateringEndTimeUnix", settings.forceWateringEndTimeUnix), dispatcher);
    addCommand(new UInt32Command("forcedWateringDuration", settings.forcedWateringDuration, 1000, 600000), dispatcher);
    addCommand(new BoolCommand("forcedWateringPerformed", settings.forcedWateringPerformed), dispatcher);

    // ============================================================
    // ПОЛИВ - MANUAL РЕЖИМ (3 команды)
    // ============================================================
    addCommand(new UInt32Command("manualWateringInterval", settings.manualWateringInterval, 60000, 86400000), dispatcher);
    addCommand(new UInt32Command("manualWateringDuration", settings.manualWateringDuration, 1000, 600000), dispatcher);
    addCommand(new UInt32Command("lastManualWateringTimeUnix", settings.lastManualWateringTimeUnix), dispatcher);

    // ============================================================
    // ПОЛИВ - AUTO РЕЖИМ (7 команд)
    // ============================================================
    addCommand(new FloatRangeCommand("matMinHumidity", settings.matMinHumidity, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("matMaxEC", settings.matMaxEC, 0.0, 10.0), dispatcher);
    addCommand(new FloatRangeCommand("radThreshold", settings.radThreshold, 0.0, 2000.0), dispatcher);
    addCommand(new FloatRangeCommand("radSum", settings.radSum, 0.0, 10000.0), dispatcher);
    addCommand(new UInt32Command("radCheckInterval", settings.radCheckInterval, 10000, 600000), dispatcher);
    addCommand(new UInt32Command("radSumResetInterval", settings.radSumResetInterval, 60000, 86400000), dispatcher);
    addCommand(new UInt32Command("lastSunTimeUnix", settings.lastSunTimeUnix), dispatcher);

    // ============================================================
    // ПОЛИВ - ВРЕМЕННЫЕ ОКНА (6 команд)
    // ============================================================
    addCommand(new UInt8RangeCommand("wateringStartHour", settings.wateringStartHour, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("wateringStartMinute", settings.wateringStartMinute, 0, 59), dispatcher);
    addCommand(new UInt8RangeCommand("wateringEndHour", settings.wateringEndHour, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("wateringEndMinute", settings.wateringEndMinute, 0, 59), dispatcher);
    addCommand(new UInt8RangeCommand("maxWateringCycles", settings.maxWateringCycles, 1, 50), dispatcher);
    addCommand(new UInt8RangeCommand("cycleCount", settings.cycleCount, 0, 255), dispatcher);

    // ============================================================
    // ПОЛИВ - ИНТЕРВАЛЫ (3 команды)
    // ============================================================
    addCommand(new UInt32Command("minWateringInterval", settings.minWateringInterval, 60000, 86400000), dispatcher);
    addCommand(new UInt32Command("maxWateringInterval", settings.maxWateringInterval, 60000, 86400000), dispatcher);
    addCommand(new UInt32Command("lastWateringStartUnix", settings.lastWateringStartUnix), dispatcher);

    // ============================================================
    // ПОЛИВ - MORNING РЕЖИМ (9 команд)
    // ============================================================
    addCommand(new UInt8RangeCommand("morningWateringCount", settings.morningWateringCount, 0, 10), dispatcher);
    addCommand(new UInt32Command("morningWateringInterval", settings.morningWateringInterval, 60000, 3600000), dispatcher);
    addCommand(new UInt32Command("morningWateringDuration", settings.morningWateringDuration, 1000, 600000), dispatcher);
    addCommand(new BoolCommand("morningStartedToday", settings.morningStartedToday), dispatcher);
    addCommand(new BoolCommand("morningWateringActive", settings.morningWateringActive), dispatcher);
    addCommand(new UInt8RangeCommand("currentMorningWatering", settings.currentMorningWatering, 0, 10), dispatcher);
    addCommand(new UInt32Command("lastMorningWateringStartUnix", settings.lastMorningWateringStartUnix), dispatcher);
    addCommand(new UInt32Command("lastMorningWateringEndUnix", settings.lastMorningWateringEndUnix), dispatcher);
    addCommand(new BoolCommand("pendingMorningComplete", settings.pendingMorningComplete), dispatcher);

    // ============================================================
    // ПРОЧИЕ (1 команда)
    // ============================================================
    addCommand(new IntRangeCommand("prevWateringMode", settings.prevWateringMode, -1, 3), dispatcher);

    // Примечание: wateringModes[4] требует специальной обработки (массив структур)
    // Это будет реализовано отдельно в handlers
  }

  /**
   * @brief Получить количество зарегистрированных команд
   */
  size_t getCommandCount() const {
    return commands.size();
  }
};

#endif // ALL_COMMANDS_H
