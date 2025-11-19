#!/usr/bin/env python3
"""
Fix database migration to remove pump_mode (doesn't exist in ESP32)
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/database-migrations.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим миграционную ноду
for node in workflow['nodes']:
    if node['name'] == '4. Добавить режимы оборудования':
        query = node['parameters']['query']

        # Удаляем pump_mode из ALTER TABLE
        query = query.replace(
            'ADD COLUMN IF NOT EXISTS pump_mode VARCHAR(20) DEFAULT \'auto\',',
            ''
        )

        # Очищаем пустые строки
        query = '\n'.join([line for line in query.split('\n') if line.strip()])

        node['parameters']['query'] = query
        print("✓ Обновлена миграция '4. Добавить режимы оборудования'")
        print("\nУдалено:")
        print("  - pump_mode (не существует в ESP32)")
        print("\nОстались режимы:")
        print("  - fan_mode (вентилятор)")
        print("  - heat_mode (отопление)")
        print("  - fog_mode (туман)")
        print("  - hydro_mix_mode (гидроразмешивание)")
        print("\nПримечание:")
        print("  - watering_mode уже существует в init-greenhouse-db.sql как INTEGER")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/database-migrations.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Migration workflow обновлен успешно!")
