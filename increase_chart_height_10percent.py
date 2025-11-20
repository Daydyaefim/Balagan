#!/usr/bin/env python3
"""
Increase chart height by 10%
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # Увеличиваем высоту на 10%
        old_heights = '''        /* График - фиксированная высота, БЕЗ padding */
        .chart-display {
            height: 500px;
            padding: 15px;
        }

        .chart-display canvas {
            width: 100% !important;
            height: 100% !important;
        }

        @media (max-width: 768px) {
            .chart-display {
                height: 450px;
            }
        }

        @media (max-width: 480px) {
            .chart-display {
                height: 400px;
            }
        }'''

        new_heights = '''        /* График - фиксированная высота, БЕЗ padding */
        .chart-display {
            height: 550px;
            padding: 15px;
        }

        .chart-display canvas {
            width: 100% !important;
            height: 100% !important;
        }

        @media (max-width: 768px) {
            .chart-display {
                height: 495px;
            }
        }

        @media (max-width: 480px) {
            .chart-display {
                height: 440px;
            }
        }'''

        js_code = js_code.replace(old_heights, new_heights)

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Высота графика увеличена на 10%!")
print("\nНовые размеры:")
print("  • Десктоп: 550px (было 500px)")
print("  • Планшет: 495px (было 450px)")
print("  • Телефон: 440px (было 400px)")
