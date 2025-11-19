#!/usr/bin/env python3
"""
Split chart area into fixed controls section and flexible graph section
Remove tabs, keep everything visible
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # 1. Убираем CSS вкладок и добавляем CSS для разделения
        old_tab_css = '''        /* Стили для вкладок */
        .chart-tabs {
            display: flex;
            gap: 10px;
            margin-bottom: 15px;
            border-bottom: 2px solid var(--border-color);
        }

        .chart-tab {
            padding: 10px 20px;
            background: none;
            border: none;
            color: var(--text-secondary);
            cursor: pointer;
            font-size: 15px;
            font-weight: 500;
            transition: all 0.2s;
            border-bottom: 3px solid transparent;
        }

        .chart-tab:hover {
            color: var(--text-primary);
            background: var(--bg-secondary);
        }

        .chart-tab.active {
            color: #28a745;
            border-bottom-color: #28a745;
        }

        .tab-content {
            display: none;
        }

        .tab-content.active {
            display: block;
        }

        .chart-display {
            min-height: 400px;
        }

        @media (max-width: 768px) {
            .chart-display {
                min-height: 350px;
            }
        }'''

        new_split_css = '''        /* Разделение области графика */
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

        js_code = js_code.replace(old_tab_css, new_split_css)

        # 2. Возвращаем структуру без вкладок
        old_structure = '''                    <div class="card-body">
                        <!-- Вкладки -->
                        <div class="chart-tabs">
                            <button class="chart-tab active" onclick="switchTab('graph')">
                                <i class="bi bi-graph-up"></i> График
                            </button>
                            <button class="chart-tab" onclick="switchTab('settings')">
                                <i class="bi bi-sliders"></i> Настройки
                            </button>
                        </div>

                        <!-- Вкладка: График -->
                        <div id="tab-graph" class="tab-content active">
                            <div class="chart-display">
                                <canvas id="mainChart" style="max-height: 500px;"></canvas>
                            </div>
                        </div>

                        <!-- Вкладка: Настройки -->
                        <div id="tab-settings" class="tab-content">
                        <div class="metric-checkboxes">'''

        new_structure = '''                    <div class="card-body">
                        <div class="chart-container">
                            <!-- Фиксированная секция: Кнопки управления -->
                            <div class="chart-controls">
                        <div class="metric-checkboxes">'''

        js_code = js_code.replace(old_structure, new_structure)

        # 3. Закрываем контролы и открываем секцию графика
        old_canvas_section = '''                        </div>
                        </div><!-- Закрытие tab-settings -->
                    </div>'''

        new_canvas_section = '''                        </div>
                            </div><!-- Закрытие chart-controls -->

                            <!-- Гибкая секция: График -->
                            <div class="chart-display">
                                <canvas id="mainChart"></canvas>
                            </div>
                        </div><!-- Закрытие chart-container -->
                    </div>'''

        js_code = js_code.replace(old_canvas_section, new_canvas_section)

        # 4. Убираем функцию switchTab
        js_code = js_code.replace('''        // Переключение между вкладками
        function switchTab(tabName) {
            // Убираем active со всех вкладок
            document.querySelectorAll('.chart-tab').forEach(tab => {
                tab.classList.remove('active');
            });
            document.querySelectorAll('.tab-content').forEach(content => {
                content.classList.remove('active');
            });

            // Добавляем active к выбранной вкладке
            const tabButton = document.querySelector('.chart-tab[onclick*="' + tabName + '"]');
            const tabContent = document.getElementById('tab-' + tabName);

            if (tabButton) tabButton.classList.add('active');
            if (tabContent) tabContent.classList.add('active');

            // Принудительно обновляем размер графика при переключении
            if (currentChart && tabName === 'graph') {
                setTimeout(() => {
                    currentChart.resize();
                }, 100);
            }
        }

        // Обновление времени последнего обновления''', '''        // Обновление времени последнего обновления''')

        # 5. Изменяем настройки Chart.js для адаптивности
        js_code = js_code.replace(
            'maintainAspectRatio: true,',
            'maintainAspectRatio: false,'
        )

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nИзменения:")
print("  • Убраны вкладки 'График' и 'Настройки'")
print("  • Область графика разделена на 2 части:")
print("    1. Кнопки управления (фиксированная высота)")
print("    2. График (занимает оставшееся место)")
print("\nСтруктура:")
print("  ┌─────────────────────────┐")
print("  │ Кнопки (фиксировано)    │ ← flex-shrink: 0")
print("  ├─────────────────────────┤")
print("  │                         │")
print("  │   График (растёт)       │ ← flex-grow: 1")
print("  │                         │")
print("  └─────────────────────────┘")
print("\nРезультат:")
print("  • График автоматически занимает всё свободное место")
print("  • Кнопки НЕ уменьшают график")
print("  • Всё видно одновременно")
