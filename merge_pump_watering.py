#!/usr/bin/env python3
"""
Merge pump and watering into single display
- Pump state = watering ON/OFF
- Watering mode = control mode (auto/manual/forced)
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # Находим секцию оборудования и заменяем разделённые pump и watering на объединённый
        old_equipment = '''                            <div class="equipment-item">
                                <div class="equipment-icon"><i class="bi bi-droplet-fill"></i></div>
                                <div class="equipment-state">${latest.pump_state !== undefined ? (latest.pump_state ? 'ВКЛ' : 'ВЫКЛ') : 'Н/Д'}</div>
                                <div>Насос</div>
                            </div>
                            <div class="equipment-item">
                                <div class="equipment-icon"><i class="bi bi-water"></i></div>
                                <div>Полив</div>
                                ${latest.watering_mode !== undefined ? `<span class="mode-badge ${getModeClass(latest.watering_mode === 0 ? 'auto' : (latest.watering_mode === 1 ? 'manual' : 'forced'))}\">${latest.watering_mode === 0 ? 'AUTO' : (latest.watering_mode === 1 ? 'MANUAL' : 'FORCED')}</span>` : ''}
                            </div>'''

        new_equipment = '''                            <div class="equipment-item">
                                <div class="equipment-icon"><i class="bi bi-droplet-fill"></i></div>
                                <div class="equipment-state">${latest.pump_state !== undefined ? (latest.pump_state ? 'ВКЛ' : 'ВЫКЛ') : 'Н/Д'}</div>
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
print("\nОбъединено:")
print("  💧 Полив:")
print("    - Состояние: ВКЛ/ВЫКЛ (из pump_state)")
print("    - Режим: AUTO/MANUAL/FORCED (из watering_mode)")
print("\nИтого 5 элементов оборудования:")
print("  1. Вентилятор")
print("  2. Отопление")
print("  3. Полив (насос)")
print("  4. Туман")
print("  5. Гидроразмешивание")
