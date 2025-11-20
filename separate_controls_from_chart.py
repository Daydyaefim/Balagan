#!/usr/bin/env python3
"""
Move controls OUTSIDE chart container so they don't affect chart height
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # 1. Упрощаем CSS - убираем flex контейнер
        old_css = '''        /* Разделение области графика */
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

        new_css = '''        /* Кнопки управления - независимая секция */
        .chart-controls {
            padding-bottom: 15px;
            margin-bottom: 15px;
            border-bottom: 1px solid var(--border-color);
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
        }'''

        js_code = js_code.replace(old_css, new_css)

        # 2. Меняем HTML структуру - убираем chart-container
        old_html = '''                    <div class="card-body">
                        <div class="chart-container">
                            <!-- Фиксированная секция: Кнопки управления -->
                            <div class="chart-controls">'''

        new_html = '''                    <div class="card-body">
                        <!-- Кнопки управления (отдельно от графика) -->
                        <div class="chart-controls">'''

        js_code = js_code.replace(old_html, new_html)

        # 3. Закрываем кнопки и открываем график БЕЗ оборачивания в chart-container
        old_close = '''                        </div>
                            </div><!-- Закрытие chart-controls -->

                            <!-- Гибкая секция: График -->
                            <div class="chart-display">
                                <canvas id="mainChart"></canvas>
                            </div>
                        </div><!-- Закрытие chart-container -->'''

        new_close = '''                        </div>
                        </div><!-- Закрытие chart-controls -->

                        <!-- График (независимая секция) -->
                        <div class="chart-display">
                            <canvas id="mainChart"></canvas>
                        </div>'''

        js_code = js_code.replace(old_close, new_close)

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nНовая структура:")
print("  ┌─────────────────────────┐")
print("  │ Кнопки (своя секция)    │ ← НЕ влияет на график")
print("  └─────────────────────────┘")
print("  ┌─────────────────────────┐")
print("  │                         │")
print("  │   График 500px          │ ← Фиксированная высота!")
print("  │                         │")
print("  └─────────────────────────┘")
print("\nВысота графика:")
print("  • Десктоп: 500px")
print("  • Планшет: 450px")
print("  • Телефон: 400px")
print("\nКНОПКИ НЕ ВЛИЯЮТ НА РАЗМЕР ГРАФИКА!")
