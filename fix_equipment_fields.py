#!/usr/bin/env python3
"""
Fix equipment field names in workflow to match database schema
and add hydro_mix (Гидроразмешивание) display
"""

import json
import sys

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Подготовить Данные"
for node in workflow['nodes']:
    if node['name'] == 'Подготовить Данные':
        js_code = node['parameters']['jsCode']

        # Исправляем названия полей в defaultData
        js_code = js_code.replace('heating_state: 0,', 'heat_state: 0,')
        js_code = js_code.replace("heating_mode: 'unknown',", "heat_mode: 'auto',")
        js_code = js_code.replace('mist_state: 0,', 'fog_state: 0,')
        js_code = js_code.replace("mist_mode: 'unknown'", "fog_mode: 'auto',\n  hydro_mix: 0,\n  hydro_mix_mode: 'auto'")

        # Исправляем парсинг lastReading
        js_code = js_code.replace(
            "heating_state: parseInt(lastReading.heating_state) || 0,",
            "heat_state: parseInt(lastReading.heat_state) || 0,"
        )
        js_code = js_code.replace(
            "heating_mode: lastReading.heating_mode || 'unknown',",
            "heat_mode: lastReading.heat_mode || 'auto',"
        )
        js_code = js_code.replace(
            "mist_state: parseInt(lastReading.mist_state) || 0,",
            "fog_state: parseInt(lastReading.fog_state) || 0,"
        )
        js_code = js_code.replace(
            "mist_mode: lastReading.mist_mode || 'unknown'",
            "fog_mode: lastReading.fog_mode || 'auto',\n    hydro_mix: parseInt(lastReading.hydro_mix) || 0,\n    hydro_mix_mode: lastReading.hydro_mix_mode || 'auto'"
        )

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Подготовить Данные'")

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # Исправляем ссылки на heating в HTML
        js_code = js_code.replace('latest.heating_state', 'latest.heat_state')
        js_code = js_code.replace('latest.heating_mode', 'latest.heat_mode')
        js_code = js_code.replace('latest.mist_state', 'latest.fog_state')
        js_code = js_code.replace('latest.mist_mode', 'latest.fog_mode')

        # Добавляем Гидроразмешивание в HTML (после Туман)
        old_equipment_html = '''                            <div class="equipment-item">
                                <div class="equipment-icon"><i class="bi bi-cloud-drizzle"></i></div>
                                <div class="equipment-state">${latest.fog_state !== undefined ? (latest.fog_state ? 'ВКЛ' : 'ВЫКЛ') : 'Н/Д'}</div>
                                <div>Туман</div>
                                ${latest.fog_mode ? `<span class="mode-badge ${getModeClass(latest.fog_mode)}">${latest.fog_mode.toUpperCase()}</span>` : ''}
                            </div>
                        </div>'''

        new_equipment_html = '''                            <div class="equipment-item">
                                <div class="equipment-icon"><i class="bi bi-cloud-drizzle"></i></div>
                                <div class="equipment-state">${latest.fog_state !== undefined ? (latest.fog_state ? 'ВКЛ' : 'ВЫКЛ') : 'Н/Д'}</div>
                                <div>Туман</div>
                                ${latest.fog_mode ? `<span class="mode-badge ${getModeClass(latest.fog_mode)}">${latest.fog_mode.toUpperCase()}</span>` : ''}
                            </div>
                            <div class="equipment-item">
                                <div class="equipment-icon"><i class="bi bi-tsunami"></i></div>
                                <div class="equipment-state">${latest.hydro_mix !== undefined ? (latest.hydro_mix ? 'ВКЛ' : 'ВЫКЛ') : 'Н/Д'}</div>
                                <div>Гидроразмешивание</div>
                                ${latest.hydro_mix_mode ? `<span class="mode-badge ${getModeClass(latest.hydro_mix_mode)}">${latest.hydro_mix_mode.toUpperCase()}</span>` : ''}
                            </div>
                        </div>'''

        js_code = js_code.replace(old_equipment_html, new_equipment_html)

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nИзменения:")
print("  - heating_state → heat_state")
print("  - heating_mode → heat_mode")
print("  - mist_state → fog_state")
print("  - mist_mode → fog_mode")
print("  - Добавлено: hydro_mix (Гидроразмешивание)")
print("  - Добавлено: hydro_mix_mode")
