#!/usr/bin/env python3
"""
Create tabs for chart: one for graph display, one for settings
Remove fullscreen buttons
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # 1. Убираем CSS для fullscreen
        js_code = js_code.replace('''        /* Стили для полноэкранного режима */
        .fullscreen-btn {
            background: var(--gradient-info);
            border: none;
            color: white;
            padding: 8px 16px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 14px;
            display: flex;
            align-items: center;
            gap: 6px;
            transition: all 0.2s;
        }

        .fullscreen-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0,0,0,0.2);
        }

        .chart-fullscreen {
            position: fixed !important;
            top: 0 !important;
            left: 0 !important;
            width: 100vw !important;
            height: 100vh !important;
            z-index: 9999 !important;
            background: var(--bg-primary) !important;
            padding: 20px !important;
            margin: 0 !important;
        }

        .chart-fullscreen canvas {
            height: calc(100vh - 80px) !important;
            max-height: none !important;
        }

        .chart-fullscreen .metric-checkboxes,
        .chart-fullscreen .time-controls,
        .chart-fullscreen .card-header button {
            display: none !important;
        }

        .chart-fullscreen .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .close-fullscreen-btn {
            background: var(--gradient-danger);
            border: none;
            color: white;
            padding: 8px 16px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 16px;
            display: none;
        }

        .chart-fullscreen .close-fullscreen-btn {
            display: block !important;
        }''', '''        /* Стили для вкладок */
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
        }''')

        # 2. Убираем кнопки fullscreen из заголовка
        js_code = js_code.replace('''                        <div class="d-flex justify-content-between align-items-center flex-wrap gap-2">
                            <h5 class="mb-0"><i class="bi bi-graph-up"></i> Графики Показаний</h5>
                            <div class="d-flex gap-2">
                                <button class="fullscreen-btn" onclick="toggleFullscreen()">
                                    <i class="bi bi-arrows-fullscreen"></i> Развернуть
                                </button>
                                <button class="close-fullscreen-btn" onclick="toggleFullscreen()">
                                    <i class="bi bi-x-lg"></i> Закрыть
                                </button>
                                <button class="btn btn-sm btn-success" onclick="exportCSV()">
                                    <i class="bi bi-download"></i> Экспорт CSV
                                </button>
                            </div>
                        </div>''', '''                        <div class="d-flex justify-content-between align-items-center flex-wrap gap-2">
                            <h5 class="mb-0"><i class="bi bi-graph-up"></i> Графики Показаний</h5>
                            <button class="btn btn-sm btn-success" onclick="exportCSV()">
                                <i class="bi bi-download"></i> Экспорт CSV
                            </button>
                        </div>''')

        # 3. Создаём структуру с вкладками
        old_chart_body = '''                    <div class="card-body">
                        <div class="metric-checkboxes">'''

        new_chart_body = '''                    <div class="card-body">
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

        js_code = js_code.replace(old_chart_body, new_chart_body)

        # 4. Закрываем вкладку настроек после time controls и перед canvas
        old_canvas = '''                        </div>
                        <canvas id="mainChart" style="max-height: 400px;"></canvas>
                    </div>'''

        new_canvas = '''                        </div>
                        </div><!-- Закрытие tab-settings -->
                    </div>'''

        js_code = js_code.replace(old_canvas, new_canvas)

        # 5. Убираем функцию toggleFullscreen и добавляем switchTab
        js_code = js_code.replace('''        // Полноэкранный режим графика
        function toggleFullscreen() {
            const chartCard = document.querySelector('.chart-card');
            chartCard.classList.toggle('chart-fullscreen');

            // Принудительно обновляем размер графика
            if (currentChart) {
                setTimeout(() => {
                    currentChart.resize();
                }, 100);
            }
        }

        // Обновление времени последнего обновления''', '''        // Переключение между вкладками
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

        // Обновление времени последнего обновления''')

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nСоздана система вкладок:")
print("  📊 Вкладка 'График' - только график (500px высота)")
print("  ⚙️ Вкладка 'Настройки' - чекбоксы + кнопки времени")
print("\nУдалено:")
print("  ✕ Кнопка 'Развернуть'")
print("  ✕ Кнопка 'Закрыть'")
print("  ✕ CSS полноэкранного режима")
print("\nПреимущества:")
print("  • График не загромождён кнопками")
print("  • Больше места для графика на мобильных")
print("  • Удобное переключение между просмотром и настройкой")
