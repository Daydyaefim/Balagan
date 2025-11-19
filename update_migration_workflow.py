#!/usr/bin/env python3
"""
Add equipment modes migration to database-migrations workflow
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/database-migrations.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Создаём новую ноду для миграции режимов оборудования
equipment_modes_node = {
    "parameters": {
        "operation": "executeQuery",
        "query": """-- Добавление столбцов для режимов работы оборудования
ALTER TABLE ugagro_readings
    ADD COLUMN IF NOT EXISTS fan_mode VARCHAR(20) DEFAULT 'auto',
    ADD COLUMN IF NOT EXISTS heat_mode VARCHAR(20) DEFAULT 'auto',
    ADD COLUMN IF NOT EXISTS pump_mode VARCHAR(20) DEFAULT 'auto',
    ADD COLUMN IF NOT EXISTS fog_mode VARCHAR(20) DEFAULT 'auto',
    ADD COLUMN IF NOT EXISTS hydro_mix_mode VARCHAR(20) DEFAULT 'auto';

-- Добавляем столбец для солнечной радиации (алиас для pyrano)
ALTER TABLE ugagro_readings
    ADD COLUMN IF NOT EXISTS solar_radiation DECIMAL(5,2);

-- Создаем триггер для синхронизации solar_radiation и pyrano
CREATE OR REPLACE FUNCTION sync_solar_radiation() RETURNS TRIGGER AS $$
BEGIN
    IF NEW.pyrano IS NOT NULL THEN
        NEW.solar_radiation := NEW.pyrano;
    END IF;
    IF NEW.solar_radiation IS NOT NULL THEN
        NEW.pyrano := NEW.solar_radiation;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trigger_sync_solar_radiation ON ugagro_readings;
CREATE TRIGGER trigger_sync_solar_radiation
    BEFORE INSERT OR UPDATE ON ugagro_readings
    FOR EACH ROW
    EXECUTE FUNCTION sync_solar_radiation();

-- Обновляем существующие записи
UPDATE ugagro_readings SET solar_radiation = pyrano WHERE solar_radiation IS NULL;

SELECT 'equipment_modes_migration_completed' as result;""",
        "options": {}
    },
    "id": "add-equipment-modes",
    "name": "4. Добавить режимы оборудования",
    "type": "n8n-nodes-base.postgres",
    "typeVersion": 2.4,
    "position": [900, 300],
    "credentials": {
        "postgres": {
            "id": "greenhouse_db",
            "name": "Greenhouse PostgreSQL"
        }
    }
}

# Обновляем существующую проверочную ноду
verify_node = None
for i, node in enumerate(workflow['nodes']):
    if node['id'] == 'verify-migrations':
        # Переименовываем и меняем позицию
        node['name'] = '5. Проверить Результаты'
        node['position'] = [1120, 300]
        verify_node = node

        # Обновляем запрос проверки
        node['parameters']['query'] = """-- Проверка созданных таблиц и данных
SELECT 'watering_settings' as table_name, COUNT(*) as row_count FROM watering_settings
UNION ALL
SELECT 'watering_notifications' as table_name, COUNT(*) as row_count FROM watering_notifications
UNION ALL
SELECT 'telegram_alert_states (esp32_connection_lost)' as table_name, COUNT(*) as row_count
FROM telegram_alert_states WHERE alert_type = 'esp32_connection_lost'
UNION ALL
SELECT 'ugagro_readings (equipment modes)' as table_name,
       COUNT(*) FILTER (WHERE fan_mode IS NOT NULL) as row_count
FROM ugagro_readings
UNION ALL
SELECT 'ugagro_readings (solar_radiation)' as table_name,
       COUNT(*) FILTER (WHERE solar_radiation IS NOT NULL) as row_count
FROM ugagro_readings;"""
    elif node['id'] == 'format-results':
        # Обновляем позицию финальной ноды
        node['position'] = [1340, 300]

# Вставляем новую ноду после esp32_connection_lost
workflow['nodes'].insert(4, equipment_modes_node)

# Обновляем connections
# Старая связь: esp32_connection_lost -> verify-migrations
# Новая связь: esp32_connection_lost -> add-equipment-modes -> verify-migrations

# Находим и обновляем связь от esp32_connection_lost
workflow['connections']['3. Добавить esp32_connection_lost'] = {
    "main": [[{
        "node": "4. Добавить режимы оборудования",
        "type": "main",
        "index": 0
    }]]
}

# Добавляем связь от новой ноды к verify
workflow['connections']['4. Добавить режимы оборудования'] = {
    "main": [[{
        "node": "5. Проверить Результаты",
        "type": "main",
        "index": 0
    }]]
}

# Обновляем связь от verify к format (переименовываем ключ)
old_verify_connection = workflow['connections']['4. Проверить Результаты']
workflow['connections']['5. Проверить Результаты'] = old_verify_connection
del workflow['connections']['4. Проверить Результаты']

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/database-migrations.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("✅ Migration workflow обновлен!")
print("\nДобавлена нода:")
print("  • 4. Добавить режимы оборудования")
print("    - fan_mode, heat_mode, pump_mode, fog_mode, hydro_mix_mode")
print("    - solar_radiation с автосинхронизацией с pyrano")
print("\nОбновлена нода:")
print("  • 5. Проверить Результаты (с проверкой новых полей)")
