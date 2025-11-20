#!/usr/bin/env python3
"""
Fix watering vs pump display
- Pump = physical state (ON/OFF), no mode
- Watering = logical system with modes (auto/manual/forced)
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Подготовить Данные"
for node in workflow['nodes']:
    if node['name'] == 'Подготовить Данные':
        js_code = node['parameters']['jsCode']

        # Добавляем watering_mode в defaultData
        js_code = js_code.replace(
            'pump_mode: \'unknown\',',
            'watering_mode: 0,'
        )

        # Добавляем watering_mode в latest
        js_code = js_code.replace(
            'pump_mode: lastReading.pump_mode || \'unknown\',',
            'watering_mode: parseInt(lastReading.watering_mode) || 0,'
        )

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Подготовить Данные'")

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # Находим секцию оборудования и заменяем
        old_equipment = '''                            <div class="equipment-item">
                                <div class="equipment-icon"><i class="bi bi-droplet-fill"></i></div>
                                <div class="equipment-state">${latest.pump_state !== undefined ? (latest.pump_state ? 'ВКЛ' : 'ВЫКЛ') : 'Н/Д'}</div>
                                <div>Насос</div>
                                ${latest.pump_mode ? `<span class="mode-badge ${getModeClass(latest.pump_mode)}\">${latest.pump_mode.toUpperCase()}</span>` : ''}
                            </div>'''

        new_equipment = '''                            <div class="equipment-item">
                                <div class="equipment-icon"><i class="bi bi-droplet-fill"></i></div>
                                <div class="equipment-state">${latest.pump_state !== undefined ? (latest.pump_state ? 'ВКЛ' : 'ВЫКЛ') : 'Н/Д'}</div>
                                <div>Насос</div>
                            </div>
                            <div class="equipment-item">
                                <div class="equipment-icon"><i class="bi bi-water"></i></div>
                                <div>Полив</div>
                                ${latest.watering_mode !== undefined ? `<span class="mode-badge ${getModeClass(latest.watering_mode === 0 ? 'auto' : (latest.watering_mode === 1 ? 'manual' : 'forced'))}\">${latest.watering_mode === 0 ? 'AUTO' : (latest.watering_mode === 1 ? 'MANUAL' : 'FORCED')}</span>` : ''}
                            </div>'''

        js_code = js_code.replace(old_equipment, new_equipment)

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nИзменения:")
print("  • Насос - только состояние ВКЛ/ВЫКЛ (без режима)")
print("  • Полив - отдельный элемент с режимами:")
print("    - 0 = AUTO")
print("    - 1 = MANUAL")
print("    - 2 = FORCED")
