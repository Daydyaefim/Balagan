#!/usr/bin/env python3
"""
Fix MQTT pipeline to use watering_mode instead of pump_mode
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/01-mqtt-data-pipeline.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим PostgreSQL ноду
for node in workflow['nodes']:
    if node['name'] == 'Сохранить в PostgreSQL':
        query = node['parameters']['query']

        # Заменяем pump_mode на watering_mode в списке столбцов
        query = query.replace(
            'fan_mode, heat_mode, pump_mode, fog_mode, hydro_mix_mode,',
            'fan_mode, heat_mode, fog_mode, hydro_mix_mode,'
        )

        # Заменяем значение pump_mode на watering_mode
        query = query.replace(
            "'{{ $json.fan_mode || \"auto\" }}',\n  '{{ $json.heat_mode || \"auto\" }}',\n  '{{ $json.pump_mode || \"auto\" }}',\n  '{{ $json.fog_mode || \"auto\" }}',\n  '{{ $json.hydro_mix_mode || \"auto\" }}',",
            "'{{ $json.fan_mode || \"auto\" }}',\n  '{{ $json.heat_mode || \"auto\" }}',\n  '{{ $json.fog_mode || \"auto\" }}',\n  '{{ $json.hydro_mix_mode || \"auto\" }}',"
        )

        node['parameters']['query'] = query
        print("✓ Обновлена нода 'Сохранить в PostgreSQL'")
        print("\nУдалено:")
        print("  - pump_mode (не существует в ESP32)")
        print("\nПримечание:")
        print("  - watering_mode уже существует в таблице как INTEGER")
        print("  - ESP32 отправляет watering_mode как часть основных данных")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/01-mqtt-data-pipeline.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
