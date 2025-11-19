#!/usr/bin/env python3
"""
Fix chart container height - make it taller on desktop and mobile
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # Заменяем CSS для высоты контейнера
        old_height = '''        /* Разделение области графика */
        .chart-container {
            display: flex;
            flex-direction: column;
            height: 600px;
        }

        .chart-controls {
            flex-shrink: 0;
            padding-bottom: 15px;
            border-bottom: 1px solid var(--border-color);
        }

        .chart-display {
            flex-grow: 1;
            min-height: 0;
            padding-top: 15px;
        }

        .chart-display canvas {
            width: 100% !important;
            height: 100% !important;
            max-height: none !important;
        }

        @media (max-width: 768px) {
            .chart-container {
                height: 500px;
            }
        }'''

        new_height = '''        /* Разделение области графика */
        .chart-container {
            display: flex;
            flex-direction: column;
            height: 700px;
        }

        .chart-controls {
            flex-shrink: 0;
            padding-bottom: 15px;
            border-bottom: 1px solid var(--border-color);
        }

        .chart-display {
            flex-grow: 1;
            min-height: 0;
            padding-top: 15px;
        }

        .chart-display canvas {
            width: 100% !important;
            height: 100% !important;
            max-height: none !important;
        }

        @media (max-width: 768px) {
            .chart-container {
                height: 70vh;
                min-height: 500px;
            }
        }

        @media (max-width: 480px) {
            .chart-container {
                height: 75vh;
                min-height: 550px;
            }
        }'''

        js_code = js_code.replace(old_height, new_height)

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nНовые размеры контейнера:")
print("  • Десктоп: 700px высота")
print("  • Планшет (< 768px): 70vh (70% высоты экрана), минимум 500px")
print("  • Телефон (< 480px): 75vh (75% высоты экрана), минимум 550px")
print("\nРезультат:")
print("  • На компьютере график стал выше (700px вместо 600px)")
print("  • На телефоне график занимает 70-75% высоты экрана")
print("  • График автоматически подстраивается под размер экрана")
