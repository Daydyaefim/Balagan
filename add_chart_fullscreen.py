#!/usr/bin/env python3
"""
Revert chart size and add fullscreen button for chart expansion
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # 1. Возвращаем прежние CSS настройки графика
        old_chart_css = '''        /* Адаптивность графика для мобильных */
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

        new_chart_css = '''        /* Стили для полноэкранного режима */
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
        }'''

        js_code = js_code.replace(old_chart_css, new_chart_css)

        # 2. Возвращаем прежние настройки Chart.js
        js_code = js_code.replace(
            'maintainAspectRatio: false,',
            'maintainAspectRatio: true,'
        )

        # 3. Возвращаем прежний style для canvas
        js_code = js_code.replace(
            '<canvas id="mainChart"></canvas>',
            '<canvas id="mainChart" style="max-height: 400px;"></canvas>'
        )

        # 4. Добавляем кнопку полноэкранного режима в заголовок графика
        old_header = '''                        <div class="d-flex justify-content-between align-items-center flex-wrap gap-2">
                            <h5 class="mb-0"><i class="bi bi-graph-up"></i> Графики Показаний</h5>
                            <button class="btn btn-sm btn-success" onclick="exportCSV()">
                                <i class="bi bi-download"></i> Экспорт CSV
                            </button>
                        </div>'''

        new_header = '''                        <div class="d-flex justify-content-between align-items-center flex-wrap gap-2">
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
                        </div>'''

        js_code = js_code.replace(old_header, new_header)

        # 5. Добавляем JavaScript функцию для полноэкранного режима
        old_update_time = '''        // Обновление времени последнего обновления
        function updateLastUpdateTime() {
            const now = new Date();
            document.getElementById('lastUpdate').textContent = now.toLocaleString('ru-RU');
        }'''

        new_update_time = '''        // Полноэкранный режим графика
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

        // Обновление времени последнего обновления
        function updateLastUpdateTime() {
            const now = new Date();
            document.getElementById('lastUpdate').textContent = now.toLocaleString('ru-RU');
        }'''

        js_code = js_code.replace(old_update_time, new_update_time)

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nИзменения:")
print("  • Возвращён прежний размер графика (max-height: 400px)")
print("  • Добавлена кнопка '🔲 Развернуть' около графика")
print("  • В полноэкранном режиме:")
print("    - График на весь экран")
print("    - Скрыты чекбоксы и кнопки времени")
print("    - Показана только кнопка '✕ Закрыть'")
print("    - Видны только линии графиков")
