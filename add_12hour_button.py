#!/usr/bin/env python3
"""
Add 12 hours interval button to time controls
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # Добавляем кнопку 12 часов в HTML
        old_buttons = '''                        <div class="time-controls">
                            <button class="time-btn active" onclick="setTimeRange('hour')" data-range="hour">1 час</button>
                            <button class="time-btn" onclick="setTimeRange('day')" data-range="day">1 день</button>'''

        new_buttons = '''                        <div class="time-controls">
                            <button class="time-btn active" onclick="setTimeRange('hour')" data-range="hour">1 час</button>
                            <button class="time-btn" onclick="setTimeRange('12hours')" data-range="12hours">12 часов</button>
                            <button class="time-btn" onclick="setTimeRange('day')" data-range="day">1 день</button>'''

        js_code = js_code.replace(old_buttons, new_buttons)

        # Добавляем обработку 12 часов в функцию filterDataByTime
        old_switch = '''                    switch(currentTimeRange) {
                        case 'hour':
                            include = diffHours <= 1;
                            break;
                        case 'day':
                            include = diffHours <= 24;
                            break;'''

        new_switch = '''                    switch(currentTimeRange) {
                        case 'hour':
                            include = diffHours <= 1;
                            break;
                        case '12hours':
                            include = diffHours <= 12;
                            break;
                        case 'day':
                            include = diffHours <= 24;
                            break;'''

        js_code = js_code.replace(old_switch, new_switch)

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Кнопка '12 часов' добавлена!")
print("\nПорядок кнопок:")
print("  [1 час] [12 часов] [1 день] [3 дня] [Неделя] [Все данные]")
