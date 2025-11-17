#ifndef ALL_COMMANDS_H
#define ALL_COMMANDS_H

#include "CommandDispatcher.h"
#include "SettingCommands.h"
#include "../Settings.h"
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
   * @brief Регистрация всех команд
   */
  void registerAllCommands(CommandDispatcher& dispatcher) {
    // ============================================================
    // КЛИМАТ (4 команды)
    // ============================================================
    addCommand(new FloatRangeCommand("minTemp", settings.climate.minTemp, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("maxTemp", settings.climate.maxTemp, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("minHum", settings.climate.minHum, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("maxHum", settings.climate.maxHum, 0.0, 100.0), dispatcher);

    // ============================================================
    // ОКНА (5 команд)
    // ============================================================
    addCommand(new UInt32Command("openTime", settings.window.openTime, 1000, 300000), dispatcher);
    addCommand(new UInt32Command("closeTime", settings.window.closeTime, 1000, 300000), dispatcher);
    addCommand(new BoolCommand("manualWindow", settings.window.manualWindow), dispatcher);
    // windowCommand и windowDuration обрабатываются отдельно (специальная логика)

    // ============================================================
    // ОТОПЛЕНИЕ (4 команды)
    // ============================================================
    addCommand(new FloatRangeCommand("heatOn", settings.heating.heatOn, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("heatOff", settings.heating.heatOff, -20.0, 50.0), dispatcher);
    addCommand(new BoolCommand("manualHeat", settings.heating.manualHeat), dispatcher);
    addCommand(new BoolCommand("heatState", settings.heating.heatState), dispatcher);

    // ============================================================
    // ВЕНТИЛЯТОР (4 команды)
    // ============================================================
    addCommand(new UInt32Command("fanInterval", settings.fan.fanInterval, 60000, 86400000), dispatcher);
    addCommand(new UInt32Command("fanDuration", settings.fan.fanDuration, 1000, 3600000), dispatcher);
    addCommand(new BoolCommand("manualFan", settings.fan.manualFan), dispatcher);
    addCommand(new BoolCommand("fanState", settings.fan.fanState), dispatcher);

    // ============================================================
    // ТУМАН (22 команды)
    // ============================================================
    addCommand(new IntRangeCommand("fogMode", settings.fog.fogMode, 0, 2), dispatcher);

    addCommand(new UInt8RangeCommand("fogMorningStart", settings.fog.fogMorningStart, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("fogMorningEnd", settings.fog.fogMorningEnd, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("fogDayStart", settings.fog.fogDayStart, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("fogDayEnd", settings.fog.fogDayEnd, 0, 23), dispatcher);

    addCommand(new UInt32Command("fogMorningDuration", settings.fog.fogMorningDuration, 1000, 60000), dispatcher);
    addCommand(new UInt32Command("fogMorningInterval", settings.fog.fogMorningInterval, 10000, 3600000), dispatcher);
    addCommand(new UInt32Command("fogDayDuration", settings.fog.fogDayDuration, 1000, 60000), dispatcher);
    addCommand(new UInt32Command("fogDayInterval", settings.fog.fogDayInterval, 10000, 3600000), dispatcher);
    addCommand(new UInt32Command("fogDelay", settings.fog.fogDelay, 0, 10000), dispatcher);

    addCommand(new FloatRangeCommand("fogMinHum", settings.fog.fogMinHum, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("fogMaxHum", settings.fog.fogMaxHum, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("fogMinTemp", settings.fog.fogMinTemp, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("fogMaxTemp", settings.fog.fogMaxTemp, -20.0, 50.0), dispatcher);

    addCommand(new UInt32Command("fogMinDuration", settings.fog.fogMinDuration, 1000, 60000), dispatcher);
    addCommand(new UInt32Command("fogMaxDuration", settings.fog.fogMaxDuration, 1000, 60000), dispatcher);
    addCommand(new UInt32Command("fogMinInterval", settings.fog.fogMinInterval, 10000, 3600000), dispatcher);
    addCommand(new UInt32Command("fogMaxInterval", settings.fog.fogMaxInterval, 10000, 3600000), dispatcher);

    addCommand(new UInt32Command("autoFogDayStart", settings.fog.autoFogDayStart, 0, 86400000), dispatcher);
    addCommand(new UInt32Command("autoFogDayEnd", settings.fog.autoFogDayEnd, 0, 86400000), dispatcher);

    addCommand(new BoolCommand("forceFogOn", settings.fog.forceFogOn), dispatcher);
    addCommand(new BoolCommand("fogState", settings.fog.fogState), dispatcher);

    // ============================================================
    // РАСТВОР (4 команды)
    // ============================================================
    addCommand(new FloatRangeCommand("solOn", settings.solution.solOn, -20.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("solOff", settings.solution.solOff, -20.0, 50.0), dispatcher);
    addCommand(new BoolCommand("manualSol", settings.solution.manualSol), dispatcher);
    addCommand(new BoolCommand("solHeatState", settings.solution.solHeatState), dispatcher);

    // ============================================================
    // УРОВЕНЬ (3 команды)
    // ============================================================
    addCommand(new FloatRangeCommand("levelMin", settings.level.levelMin, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("levelMax", settings.level.levelMax, 0.0, 100.0), dispatcher);
    addCommand(new BoolCommand("fillState", settings.level.fillState), dispatcher);

    // ============================================================
    // ПОЛИВ - БАЗОВЫЕ (6 команд)
    // ============================================================
    addCommand(new IntRangeCommand("wateringMode", settings.watering.wateringMode, 0, 2), dispatcher);
    addCommand(new UInt32Command("pumpTime", settings.watering.pumpTime, 1000, 600000), dispatcher);
    addCommand(new BoolCommand("manualPump", settings.watering.manualPump), dispatcher);
    addCommand(new BoolCommand("pumpState", settings.watering.pumpState), dispatcher);
    addCommand(new BoolCommand("manualPumpOverride", settings.watering.manualPumpOverride), dispatcher);
    addCommand(new BoolCommand("previousManualPump", settings.watering.previousManualPump), dispatcher);

    // ============================================================
    // ПОЛИВ - AUTO РЕЖИМ (7 команд)
    // ============================================================
    addCommand(new FloatRangeCommand("radThreshold", settings.watering.radThreshold, 0.0, 2000.0), dispatcher);
    addCommand(new FloatRangeCommand("radSum", settings.watering.radSum, 0.0, 10000.0), dispatcher);
    addCommand(new UInt32Command("radCheckInterval", settings.watering.radCheckInterval, 10000, 600000), dispatcher);
    addCommand(new UInt32Command("radSumResetInterval", settings.watering.radSumResetInterval, 60000, 86400000), dispatcher);
    addCommand(new FloatRangeCommand("matMinHumidity", settings.watering.matMinHumidity, 0.0, 100.0), dispatcher);
    addCommand(new FloatRangeCommand("matMaxEC", settings.watering.matMaxEC, 0.0, 10.0), dispatcher);
    addCommand(new UInt32Command("lastSunTimeUnix", settings.watering.lastSunTimeUnix), dispatcher);

    // ============================================================
    // ПОЛИВ - ВРЕМЕННЫЕ ОКНА (6 команд)
    // ============================================================
    addCommand(new UInt8RangeCommand("wateringStartHour", settings.watering.wateringStartHour, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("wateringStartMinute", settings.watering.wateringStartMinute, 0, 59), dispatcher);
    addCommand(new UInt8RangeCommand("wateringEndHour", settings.watering.wateringEndHour, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("wateringEndMinute", settings.watering.wateringEndMinute, 0, 59), dispatcher);
    addCommand(new UInt8RangeCommand("maxWateringCycles", settings.watering.maxWateringCycles, 1, 50), dispatcher);
    addCommand(new UInt8RangeCommand("cycleCount", settings.watering.cycleCount, 0, 255), dispatcher);

    // ============================================================
    // ПОЛИВ - ИНТЕРВАЛЫ (4 команды)
    // ============================================================
    addCommand(new UInt32Command("minWateringInterval", settings.watering.minWateringInterval, 60000, 86400000), dispatcher);
    addCommand(new UInt32Command("maxWateringInterval", settings.watering.maxWateringInterval, 60000, 86400000), dispatcher);
    addCommand(new UInt32Command("lastWateringStartUnix", settings.watering.lastWateringStartUnix), dispatcher);

    // ============================================================
    // ПОЛИВ - MANUAL РЕЖИМ (3 команды)
    // ============================================================
    addCommand(new UInt32Command("manualWateringInterval", settings.watering.manualWateringInterval, 60000, 86400000), dispatcher);
    addCommand(new UInt32Command("manualWateringDuration", settings.watering.manualWateringDuration, 1000, 600000), dispatcher);
    addCommand(new UInt32Command("lastManualWateringTimeUnix", settings.watering.lastManualWateringTimeUnix), dispatcher);

    // ============================================================
    // ПОЛИВ - MORNING РЕЖИМ (9 команд)
    // ============================================================
    addCommand(new UInt8RangeCommand("morningWateringCount", settings.watering.morningWateringCount, 1, 10), dispatcher);
    addCommand(new UInt32Command("morningWateringInterval", settings.watering.morningWateringInterval, 60000, 3600000), dispatcher);
    addCommand(new UInt32Command("morningWateringDuration", settings.watering.morningWateringDuration, 1000, 600000), dispatcher);
    addCommand(new BoolCommand("morningStartedToday", settings.watering.morningStartedToday), dispatcher);
    addCommand(new BoolCommand("morningWateringActive", settings.watering.morningWateringActive), dispatcher);
    addCommand(new UInt8RangeCommand("currentMorningWatering", settings.watering.currentMorningWatering, 0, 10), dispatcher);
    addCommand(new UInt32Command("lastMorningWateringStartUnix", settings.watering.lastMorningWateringStartUnix), dispatcher);
    addCommand(new UInt32Command("lastMorningWateringEndUnix", settings.watering.lastMorningWateringEndUnix), dispatcher);
    addCommand(new BoolCommand("pendingMorningComplete", settings.watering.pendingMorningComplete), dispatcher);

    // ============================================================
    // ПОЛИВ - FORCED РЕЖИМ (7 команд)
    // ============================================================
    addCommand(new BoolCommand("forceWateringActive", settings.watering.forceWateringActive), dispatcher);
    addCommand(new BoolCommand("forceWateringOverride", settings.watering.forceWateringOverride), dispatcher);
    addCommand(new UInt32Command("forceWateringEndTime", settings.watering.forceWateringEndTime), dispatcher);
    addCommand(new UInt32Command("forceWateringEndTimeUnix", settings.watering.forceWateringEndTimeUnix), dispatcher);
    addCommand(new UInt32Command("forcedWateringDuration", settings.watering.forcedWateringDuration, 1000, 600000), dispatcher);
    addCommand(new BoolCommand("forcedWateringPerformed", settings.watering.forcedWateringPerformed), dispatcher);
    addCommand(new IntRangeCommand("prevWateringMode", settings.watering.prevWateringMode, 0, 2), dispatcher);

    // ============================================================
    // ПОЛИВ - HYDRO MIX (4 команды)
    // ============================================================
    addCommand(new BoolCommand("hydroMix", settings.watering.hydroMix), dispatcher);
    addCommand(new UInt32Command("hydroMixDuration", settings.watering.hydroMixDuration, 60000, 1800000), dispatcher);
    addCommand(new UInt32Command("hydroStart", settings.watering.hydroStart), dispatcher);
    addCommand(new UInt32Command("hydroStartUnix", settings.watering.hydroStartUnix), dispatcher);

    // ============================================================
    // ОБЩИЕ (6 команд)
    // ============================================================
    addCommand(new FloatRangeCommand("wind1", settings.general.wind1, 0.0, 50.0), dispatcher);
    addCommand(new FloatRangeCommand("wind2", settings.general.wind2, 0.0, 50.0), dispatcher);
    addCommand(new UInt32Command("windLockMinutes", settings.general.windLockMinutes, 0, 1440), dispatcher);
    addCommand(new UInt8RangeCommand("nightStart", settings.general.nightStart, 0, 23), dispatcher);
    addCommand(new UInt8RangeCommand("dayStart", settings.general.dayStart, 0, 23), dispatcher);
    addCommand(new FloatRangeCommand("nightOffset", settings.general.nightOffset, -10.0, 10.0), dispatcher);

    // Примечание: wateringModes[] требует специальной обработки (массив структур)
    // Это будет реализовано в отдельном классе WateringModesCommand
  }

  /**
   * @brief Получить количество зарегистрированных команд
   */
  size_t getCommandCount() const {
    return commands.size();
  }
};

#endif // ALL_COMMANDS_H
