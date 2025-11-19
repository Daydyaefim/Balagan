#!/usr/bin/env python3
"""
Add missing CSS for chart-controls and chart-display
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # Находим место после .online-indicator и добавляем CSS
        old_css_end = '''        .online-indicator {
            animation: pulse 2s infinite;
        }
    </style>'''

        new_css_end = '''        .online-indicator {
            animation: pulse 2s infinite;
        }

        /* Кнопки управления - независимая секция */
        .chart-controls {
            padding-bottom: 15px;
            margin-bottom: 20px;
            border-bottom: 2px solid var(--border-color);
        }

        /* График - фиксированная высота */
        .chart-display {
            height: 500px;
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
        }
    </style>'''

        js_code = js_code.replace(old_css_end, new_css_end)

        # Также нужно убедиться что Chart.js использует maintainAspectRatio: false
        js_code = js_code.replace(
            'maintainAspectRatio: true,',
            'maintainAspectRatio: false,'
        )

        node['parameters']['jsCode'] = js_code
        print("✓ Добавлен CSS для chart-controls и chart-display")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ CSS добавлен успешно!")
print("\n.chart-controls:")
print("  • padding-bottom: 15px")
print("  • margin-bottom: 20px")
print("  • border-bottom: 2px")
print("\n.chart-display:")
print("  • height: 500px (десктоп)")
print("  • height: 450px (планшет)")
print("  • height: 400px (телефон)")
print("  • canvas: 100% width and height")
print("\nmaintainAspectRatio: false")
