#!/usr/bin/env python3
"""
Update MQTT data pipeline to save equipment modes to PostgreSQL
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/01-mqtt-data-pipeline.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим PostgreSQL ноду
for node in workflow['nodes']:
    if node['name'] == 'Сохранить в PostgreSQL':
        # Текущий INSERT запрос
        old_query = node['parameters']['query']

        # Добавляем поля режимов в INSERT
        new_query = old_query.replace(
            'sol_heat_state, hydro_mix, fill_state,',
            'sol_heat_state, hydro_mix, fill_state,\n  fan_mode, heat_mode, pump_mode, fog_mode, hydro_mix_mode,'
        )

        # Добавляем значения режимов
        new_query = new_query.replace(
            '{{ $json.hydro_mix || false }},\n  {{ $json.fill_state || false }},',
            "{{ $json.hydro_mix || false }},\n  {{ $json.fill_state || false }},\n  '{{ $json.fan_mode || \"auto\" }}',\n  '{{ $json.heat_mode || \"auto\" }}',\n  '{{ $json.pump_mode || \"auto\" }}',\n  '{{ $json.fog_mode || \"auto\" }}',\n  '{{ $json.hydro_mix_mode || \"auto\" }}',"
        )

        node['parameters']['query'] = new_query
        print("✓ Обновлена нода 'Сохранить в PostgreSQL'")
        print(f"\nДобавлены поля режимов:")
        print("  - fan_mode")
        print("  - heat_mode")
        print("  - pump_mode")
        print("  - fog_mode")
        print("  - hydro_mix_mode")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/01-mqtt-data-pipeline.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nТеперь MQTT pipeline будет сохранять режимы оборудования в PostgreSQL")
