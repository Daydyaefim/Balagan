#!/usr/bin/env python3
"""
Move ALL controls OUTSIDE the chart card completely
Controls will be in separate card ABOVE chart card
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим узел "Генерация HTML"
for node in workflow['nodes']:
    if node['name'] == 'Генерация HTML':
        js_code = node['parameters']['jsCode']

        # Находим начало секции графика
        old_chart_section = '''        <!-- Unified Chart -->
        <div class="row mb-4">
            <div class="col-12">
                <div class="card chart-card">
                    <div class="card-header">
                        <div class="d-flex justify-content-between align-items-center flex-wrap gap-2">
                            <h5 class="mb-0"><i class="bi bi-graph-up"></i> Графики Показаний</h5>
                            <button class="btn btn-sm btn-success" onclick="exportCSV()">
                                <i class="bi bi-download"></i> Экспорт CSV
                            </button>
                        </div>
                    </div>
                    <div class="card-body">
                        <!-- Кнопки управления (отдельно от графика) -->
                        <div class="chart-controls">
                        <div class="metric-checkboxes">'''

        new_chart_section = '''        <!-- Controls Card (Separate from Chart) -->
        <div class="row mb-3">
            <div class="col-12">
                <div class="card chart-card">
                    <div class="card-body" style="padding: 15px;">
                        <div class="metric-checkboxes">'''

        js_code = js_code.replace(old_chart_section, new_chart_section)

        # Находим конец кнопок и начало графика
        old_chart_display = '''                        </div>
                        </div><!-- Закрытие chart-controls -->

                        <!-- График (независимая секция) -->
                        <div class="chart-display">
                            <canvas id="mainChart"></canvas>
                        </div>
                    </div>
                </div>
            </div>
        </div>'''

        new_chart_display = '''                        </div>
                    </div>
                </div>
            </div>
        </div>

        <!-- Chart Card (Separate from Controls) -->
        <div class="row mb-4">
            <div class="col-12">
                <div class="card chart-card">
                    <div class="card-header">
                        <div class="d-flex justify-content-between align-items-center flex-wrap gap-2">
                            <h5 class="mb-0"><i class="bi bi-graph-up"></i> График</h5>
                            <button class="btn btn-sm btn-success" onclick="exportCSV()">
                                <i class="bi bi-download"></i> Экспорт CSV
                            </button>
                        </div>
                    </div>
                    <div class="card-body" style="padding: 0;">
                        <div class="chart-display">
                            <canvas id="mainChart"></canvas>
                        </div>
                    </div>
                </div>
            </div>
        </div>'''

        js_code = js_code.replace(old_chart_display, new_chart_display)

        # Обновляем CSS для chart-display
        old_css = '''        /* Кнопки управления - независимая секция */
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
        }'''

        new_css = '''        /* График - фиксированная высота, БЕЗ padding */
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

        js_code = js_code.replace(old_css, new_css)

        node['parameters']['jsCode'] = js_code
        print("✓ Обновлен узел 'Генерация HTML'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/06-web-dashboard-simple.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow обновлен успешно!")
print("\nНовая структура:")
print("  ┌─────────────────────────┐")
print("  │ КАРТОЧКА 1: Кнопки      │ ← Отдельная карточка")
print("  │ - Чекбоксы метрик       │")
print("  │ - Кнопки времени        │")
print("  └─────────────────────────┘")
print("")
print("  ┌─────────────────────────┐")
print("  │ КАРТОЧКА 2: График      │ ← Отдельная карточка")
print("  │                         │")
print("  │   График 500px          │ ← ТОЛЬКО график!")
print("  │                         │")
print("  └─────────────────────────┘")
print("\nКнопки ПОЛНОСТЬЮ вне графика!")
print("График получает ПОЛНЫЕ 500px высоты!")
