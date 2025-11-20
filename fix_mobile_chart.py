#!/usr/bin/env python3
"""
Make chart responsive and larger on mobile devices
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # 1. Добавляем CSS для адаптивной высоты графика
        old_canvas_style = '<canvas id="mainChart" style="max-height: 400px;"></canvas>'
        new_canvas_style = '<canvas id="mainChart"></canvas>'

        js_code = js_code.replace(old_canvas_style, new_canvas_style)

        # 2. Добавляем медиа-запросы в CSS
        old_animation = '''        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.6; }
        }

        .online-indicator {
            animation: pulse 2s infinite;
        }'''

        new_animation = '''        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.6; }
        }

        .online-indicator {
            animation: pulse 2s infinite;
        }

        /* Адаптивность графика для мобильных */
        #mainChart {
            max-height: 400px;
            min-height: 300px;
        }

        @media (max-width: 768px) {
            #mainChart {
                max-height: 500px !important;
                min-height: 400px !important;
            }

            .metric-checkboxes {
                flex-direction: column;
                gap: 8px;
            }

            .metric-checkbox-label {
                width: 100%;
                justify-content: flex-start;
            }

            .time-controls {
                flex-direction: column;
            }

            .time-btn {
                width: 100%;
            }

            .sensor-card h2 {
                font-size: 1.5rem !important;
            }

            .equipment-status {
                grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            }
        }

        @media (max-width: 480px) {
            #mainChart {
                max-height: 600px !important;
                min-height: 450px !important;
            }
        }'''

        js_code = js_code.replace(old_animation, new_animation)

        # 3. Обновляем настройки Chart.js для адаптивности
        old_chart_options = '''                options: {
                    responsive: true,
                    maintainAspectRatio: true,'''

        new_chart_options = '''                options: {
                    responsive: true,
                    maintainAspectRatio: false,'''

        js_code = js_code.replace(old_chart_options, new_chart_options)

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nИзменения для адаптивности графика:")
print("  • Десктоп: 300-400px высота")
print("  • Планшет (< 768px): 400-500px высота")
print("  • Телефон (< 480px): 450-600px высота")
print("  • maintainAspectRatio: false (график растягивается)")
print("  • Чекбоксы и кнопки времени - вертикально на мобильных")
print("  • Оборудование - более компактная сетка")
