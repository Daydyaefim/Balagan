// ============================================================================
// UgAgro 95 - Greenhouse Dashboard Application
// ============================================================================

// ============================================================================
// Configuration
// ============================================================================
const CONFIG = {
    apiUrl: 'http://localhost:5678/webhook/greenhouse-data',
    refreshInterval: 30000, // 30 seconds
    chartMaxPoints: 100,
    dateFormat: {
        full: { year: 'numeric', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit' },
        short: { hour: '2-digit', minute: '2-digit' }
    }
};

// ============================================================================
// Data Storage (LocalStorage Wrapper)
// ============================================================================
const DataStorage = {
    keys: {
        temperature: 'ugagro_temp_data',
        humidity: 'ugagro_hum_data',
        outdoor: 'ugagro_outdoor_data',
        solar: 'ugagro_solar_data',
        matTemp: 'ugagro_mat_temp_data',
        matEc: 'ugagro_mat_ec_data',
        water: 'ugagro_water_data',
        wind: 'ugagro_wind_data',
        theme: 'ugagro_theme',
        currentMetric: 'ugagro_current_metric',
        timeRange: 'ugagro_time_range'
    },

    save(key, data) {
        try {
            localStorage.setItem(key, JSON.stringify(data));
            return true;
        } catch (e) {
            console.error('Failed to save to localStorage:', e);
            return false;
        }
    },

    load(key) {
        try {
            const data = localStorage.getItem(key);
            return data ? JSON.parse(data) : null;
        } catch (e) {
            console.error('Failed to load from localStorage:', e);
            return null;
        }
    },

    append(key, newData, maxLength = 1000) {
        const existing = this.load(key) || [];
        existing.push(newData);
        // Keep only the last maxLength items
        if (existing.length > maxLength) {
            existing.shift();
        }
        this.save(key, existing);
    },

    clear(key) {
        localStorage.removeItem(key);
    },

    clearAll() {
        Object.values(this.keys).forEach(key => this.clear(key));
    }
};

// ============================================================================
// Application State
// ============================================================================
const AppState = {
    currentMetric: 'temperature',
    timeRange: 24, // hours
    selectedMonth: null,
    selectedYear: null,
    chart: null,
    refreshTimer: null,
    lastUpdate: null,

    init() {
        // Load saved state
        this.currentMetric = DataStorage.load(DataStorage.keys.currentMetric) || 'temperature';
        this.timeRange = DataStorage.load(DataStorage.keys.timeRange) || 24;
        const savedTheme = DataStorage.load(DataStorage.keys.theme) || 'light';
        document.body.setAttribute('data-theme', savedTheme);
    },

    setMetric(metric) {
        this.currentMetric = metric;
        DataStorage.save(DataStorage.keys.currentMetric, metric);
    },

    setTimeRange(hours) {
        this.timeRange = parseInt(hours);
        DataStorage.save(DataStorage.keys.timeRange, this.timeRange);
    },

    setMonth(month) {
        this.selectedMonth = month;
    },

    setYear(year) {
        this.selectedYear = year;
    }
};

// ============================================================================
// Metric Configurations
// ============================================================================
const METRICS = {
    temperature: {
        label: 'Температура (°C)',
        color: 'rgb(255, 99, 132)',
        storageKey: DataStorage.keys.temperature,
        unit: '°C',
        getValue: (data) => parseFloat(data.temperature),
        thresholds: { min: 10, max: 40 }
    },
    humidity: {
        label: 'Влажность (%)',
        color: 'rgb(54, 162, 235)',
        storageKey: DataStorage.keys.humidity,
        unit: '%',
        getValue: (data) => parseFloat(data.humidity),
        thresholds: { min: 20, max: 85 }
    },
    outdoor: {
        label: 'Температура улицы (°C)',
        color: 'rgb(75, 192, 192)',
        storageKey: DataStorage.keys.outdoor,
        unit: '°C',
        getValue: (data) => parseFloat(data.outdoor_temp),
        thresholds: { min: -30, max: 50 }
    },
    solar: {
        label: 'Солнечная радиация (Вт/м²)',
        color: 'rgb(255, 206, 86)',
        storageKey: DataStorage.keys.solar,
        unit: 'Вт/м²',
        getValue: (data) => parseFloat(data.solar_radiation) / 10, // Divide by 10
        thresholds: { min: 0, max: 1000 }
    },
    'mat-temp': {
        label: 'Температура мата (°C)',
        color: 'rgb(153, 102, 255)',
        storageKey: DataStorage.keys.matTemp,
        unit: '°C',
        getValue: (data) => parseFloat(data.mat_temp),
        thresholds: { min: 5, max: 35 }
    },
    'mat-ec': {
        label: 'EC мата (мС/см)',
        color: 'rgb(255, 159, 64)',
        storageKey: DataStorage.keys.matEc,
        unit: 'мС/см',
        getValue: (data) => parseFloat(data.mat_ec),
        thresholds: { min: 0, max: 10 }
    },
    water: {
        label: 'Уровень воды (%)',
        color: 'rgb(0, 123, 255)',
        storageKey: DataStorage.keys.water,
        unit: '%',
        getValue: (data) => parseFloat(data.water_level),
        thresholds: { min: 5, max: 100 }
    },
    wind: {
        label: 'Скорость ветра (м/с)',
        color: 'rgb(108, 117, 125)',
        storageKey: DataStorage.keys.wind,
        unit: 'м/с',
        getValue: (data) => parseFloat(data.wind_speed),
        thresholds: { min: 0, max: 11 }
    }
};

// ============================================================================
// Chart Management
// ============================================================================
const ChartManager = {
    init() {
        const ctx = document.getElementById('unified-chart').getContext('2d');
        AppState.chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: '',
                    data: [],
                    borderColor: 'rgb(75, 192, 192)',
                    backgroundColor: 'rgba(75, 192, 192, 0.2)',
                    tension: 0.1,
                    fill: true
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        display: true,
                        position: 'top',
                        labels: {
                            color: getComputedStyle(document.body).getPropertyValue('--text-color')
                        }
                    },
                    tooltip: {
                        mode: 'index',
                        intersect: false,
                    }
                },
                scales: {
                    x: {
                        display: true,
                        title: {
                            display: true,
                            text: 'Время',
                            color: getComputedStyle(document.body).getPropertyValue('--text-color')
                        },
                        ticks: {
                            color: getComputedStyle(document.body).getPropertyValue('--text-color')
                        },
                        grid: {
                            color: getComputedStyle(document.body).getPropertyValue('--border-color')
                        }
                    },
                    y: {
                        display: true,
                        title: {
                            display: true,
                            text: 'Значение',
                            color: getComputedStyle(document.body).getPropertyValue('--text-color')
                        },
                        ticks: {
                            color: getComputedStyle(document.body).getPropertyValue('--text-color')
                        },
                        grid: {
                            color: getComputedStyle(document.body).getPropertyValue('--border-color')
                        }
                    }
                },
                interaction: {
                    mode: 'nearest',
                    axis: 'x',
                    intersect: false
                }
            }
        });
    },

    update(metric) {
        const config = METRICS[metric];
        if (!config) return;

        const data = DataStorage.load(config.storageKey) || [];
        const filteredData = this.filterDataByTimeRange(data);

        AppState.chart.data.labels = filteredData.map(item =>
            new Date(item.timestamp).toLocaleTimeString('ru-RU', CONFIG.dateFormat.short)
        );
        AppState.chart.data.datasets[0].label = config.label;
        AppState.chart.data.datasets[0].data = filteredData.map(item => item.value);
        AppState.chart.data.datasets[0].borderColor = config.color;
        AppState.chart.data.datasets[0].backgroundColor = config.color.replace('rgb', 'rgba').replace(')', ', 0.2)');

        // Update chart colors based on theme
        const textColor = getComputedStyle(document.body).getPropertyValue('--text-color');
        const borderColor = getComputedStyle(document.body).getPropertyValue('--border-color');

        AppState.chart.options.plugins.legend.labels.color = textColor;
        AppState.chart.options.scales.x.title.color = textColor;
        AppState.chart.options.scales.x.ticks.color = textColor;
        AppState.chart.options.scales.x.grid.color = borderColor;
        AppState.chart.options.scales.y.title.color = textColor;
        AppState.chart.options.scales.y.ticks.color = textColor;
        AppState.chart.options.scales.y.grid.color = borderColor;

        AppState.chart.update();
    },

    filterDataByTimeRange(data) {
        const now = new Date();
        const cutoffTime = new Date(now - AppState.timeRange * 60 * 60 * 1000);

        let filtered = data.filter(item => new Date(item.timestamp) >= cutoffTime);

        // If month and year are selected, filter by those too
        if (AppState.selectedMonth && AppState.selectedYear) {
            filtered = filtered.filter(item => {
                const date = new Date(item.timestamp);
                return date.getMonth() + 1 === parseInt(AppState.selectedMonth) &&
                       date.getFullYear() === parseInt(AppState.selectedYear);
            });
        }

        return filtered;
    }
};

// ============================================================================
// Data Fetching and Processing
// ============================================================================
const DataManager = {
    async fetchData() {
        try {
            const response = await fetch(CONFIG.apiUrl);
            if (!response.ok) throw new Error('Network response was not ok');

            const data = await response.json();
            this.processData(data);
            this.updateUI(data);
            AppState.lastUpdate = new Date();
            this.updateLastUpdateTime();
            this.updateConnectionStatus(true);
        } catch (error) {
            console.error('Error fetching data:', error);
            this.updateConnectionStatus(false);
        }
    },

    processData(data) {
        const timestamp = new Date().toISOString();

        // Store each metric
        Object.keys(METRICS).forEach(metric => {
            const config = METRICS[metric];
            try {
                const value = config.getValue(data);
                if (!isNaN(value)) {
                    DataStorage.append(config.storageKey, { timestamp, value });
                }
            } catch (e) {
                console.warn(`Failed to process ${metric}:`, e);
            }
        });

        // Update chart if currently viewing
        ChartManager.update(AppState.currentMetric);
    },

    updateUI(data) {
        // Update sensor cards
        this.updateSensorCard('temp-value', data.temperature, 10, 40);
        this.updateSensorCard('hum-value', data.humidity, 20, 85);
        this.updateSensorCard('water-value', data.water_level, 5, 100);
        this.updateSensorCard('wind-value', data.wind_speed, 0, 11);

        // Update additional readings
        document.getElementById('outdoor-temp').textContent = this.formatValue(data.outdoor_temp);
        document.getElementById('outdoor-hum').textContent = this.formatValue(data.outdoor_hum);
        document.getElementById('sol-temp').textContent = this.formatValue(data.solution_temp);
        document.getElementById('mat-temp').textContent = this.formatValue(data.mat_temp);
        document.getElementById('mat-ec').textContent = this.formatValue(data.mat_ec);
        document.getElementById('solar-radiation').textContent = this.formatValue(data.solar_radiation / 10); // Divide by 10
        document.getElementById('pyrano-value').textContent = this.formatValue(data.pyranometer);
        document.getElementById('window-pos').textContent = this.formatValue(data.window_position);

        // Update equipment states
        this.updateEquipment('fan', data.fan_state, data.fan_mode);
        this.updateEquipment('heat', data.heating_state, data.heating_mode);
        this.updateEquipment('pump', data.pump_state, data.pump_mode);
        this.updateEquipment('mist', data.mist_state, data.mist_mode);
    },

    updateSensorCard(elementId, value, min, max) {
        const element = document.getElementById(elementId);
        const cardId = elementId.replace('-value', '');
        const card = document.getElementById('card-' + cardId.replace('temp', 'temperature').replace('hum', 'humidity').replace('wind', 'wind').replace('water', 'water'));

        if (element) {
            element.textContent = this.formatValue(value);
        }

        if (card) {
            const numValue = parseFloat(value);
            card.classList.remove('status-normal', 'status-warning', 'status-critical');

            if (isNaN(numValue)) {
                card.classList.add('status-warning');
            } else if (cardId === 'water') {
                // Water level: critical below min
                if (numValue < min) {
                    card.classList.add('status-critical');
                } else {
                    card.classList.add('status-normal');
                }
            } else if (cardId === 'wind') {
                // Wind speed: warning if approaching max
                if (numValue > max * 0.8) {
                    card.classList.add('status-warning');
                } else {
                    card.classList.add('status-normal');
                }
            } else {
                // Temperature and humidity: warning if outside range
                if (numValue < min || numValue > max) {
                    card.classList.add('status-warning');
                } else {
                    card.classList.add('status-normal');
                }
            }
        }
    },

    updateEquipment(name, state, mode) {
        const stateElement = document.getElementById(`${name}-state`);
        const modeElement = document.getElementById(`${name}-mode`);

        if (stateElement) {
            const stateValue = String(state).toLowerCase();
            stateElement.textContent = this.getStateText(stateValue);
            stateElement.className = 'badge ' + this.getStateBadgeClass(stateValue);
        }

        if (modeElement) {
            const modeValue = String(mode).toLowerCase();
            modeElement.textContent = this.getModeText(modeValue);
            modeElement.className = 'badge badge-mode ' + this.getModeBadgeClass(modeValue);
        }
    },

    getStateText(state) {
        const states = {
            'on': 'ВКЛ',
            '1': 'ВКЛ',
            'true': 'ВКЛ',
            'off': 'ВЫКЛ',
            '0': 'ВЫКЛ',
            'false': 'ВЫКЛ'
        };
        return states[state] || 'Н/Д';
    },

    getStateBadgeClass(state) {
        const classes = {
            'on': 'bg-success',
            '1': 'bg-success',
            'true': 'bg-success',
            'off': 'bg-secondary',
            '0': 'bg-secondary',
            'false': 'bg-secondary'
        };
        return classes[state] || 'bg-secondary';
    },

    getModeText(mode) {
        const modes = {
            'auto': 'Авто',
            'manual': 'Ручной',
            'forced': 'Принуд.'
        };
        return modes[mode] || 'Н/Д';
    },

    getModeBadgeClass(mode) {
        const classes = {
            'auto': 'bg-info',
            'manual': 'bg-warning',
            'forced': 'bg-danger'
        };
        return classes[mode] || 'bg-secondary';
    },

    formatValue(value) {
        if (value === null || value === undefined || value === '') return '--';
        const num = parseFloat(value);
        return isNaN(num) ? '--' : num.toFixed(1);
    },

    updateLastUpdateTime() {
        const element = document.getElementById('last-update');
        if (element && AppState.lastUpdate) {
            element.textContent = AppState.lastUpdate.toLocaleString('ru-RU', CONFIG.dateFormat.full);
        }
    },

    updateConnectionStatus(connected) {
        const statusElement = document.getElementById('connection-status');
        if (statusElement) {
            if (connected) {
                statusElement.innerHTML = '<i class="bi bi-circle-fill text-success"></i> Подключено';
                statusElement.className = 'badge bg-light text-dark';
            } else {
                statusElement.innerHTML = '<i class="bi bi-circle-fill text-danger"></i> Нет связи';
                statusElement.className = 'badge bg-danger';
            }
        }
    }
};

// ============================================================================
// CSV Export
// ============================================================================
const CSVExporter = {
    export() {
        const metric = AppState.currentMetric;
        const config = METRICS[metric];
        if (!config) return;

        const data = DataStorage.load(config.storageKey) || [];
        const filteredData = ChartManager.filterDataByTimeRange(data);

        if (filteredData.length === 0) {
            alert('Нет данных для экспорта');
            return;
        }

        const csv = this.generateCSV(filteredData, config);
        this.downloadCSV(csv, `ugagro_${metric}_${Date.now()}.csv`);
    },

    generateCSV(data, config) {
        const headers = ['Дата и время', config.label];
        const rows = data.map(item => [
            new Date(item.timestamp).toLocaleString('ru-RU', CONFIG.dateFormat.full),
            item.value
        ]);

        const csvContent = [
            headers.join(','),
            ...rows.map(row => row.join(','))
        ].join('\n');

        // Add BOM for correct UTF-8 encoding in Excel
        return '\uFEFF' + csvContent;
    },

    downloadCSV(content, filename) {
        const blob = new Blob([content], { type: 'text/csv;charset=utf-8;' });
        const link = document.createElement('a');
        const url = URL.createObjectURL(blob);

        link.setAttribute('href', url);
        link.setAttribute('download', filename);
        link.style.visibility = 'hidden';

        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    }
};

// ============================================================================
// Theme Management
// ============================================================================
const ThemeManager = {
    init() {
        const savedTheme = DataStorage.load(DataStorage.keys.theme) || 'light';
        this.setTheme(savedTheme);
    },

    toggle() {
        const currentTheme = document.body.getAttribute('data-theme');
        const newTheme = currentTheme === 'dark' ? 'light' : 'dark';
        this.setTheme(newTheme);
    },

    setTheme(theme) {
        document.body.setAttribute('data-theme', theme);
        DataStorage.save(DataStorage.keys.theme, theme);
        this.updateThemeIcon(theme);

        // Update chart colors if chart exists
        if (AppState.chart) {
            ChartManager.update(AppState.currentMetric);
        }
    },

    updateThemeIcon(theme) {
        const icon = document.querySelector('#theme-toggle i');
        if (icon) {
            if (theme === 'dark') {
                icon.className = 'bi bi-sun-fill';
            } else {
                icon.className = 'bi bi-moon-stars-fill';
            }
        }
    }
};

// ============================================================================
// Event Listeners
// ============================================================================
const EventManager = {
    init() {
        // Theme toggle
        document.getElementById('theme-toggle')?.addEventListener('click', () => {
            ThemeManager.toggle();
        });

        // Metric tabs
        document.querySelectorAll('#data-tabs .nav-link').forEach(tab => {
            tab.addEventListener('click', (e) => {
                e.preventDefault();
                const metric = e.currentTarget.getAttribute('data-metric');
                this.handleMetricChange(metric);
            });
        });

        // Time range buttons
        document.querySelectorAll('input[name="timeRange"]').forEach(radio => {
            radio.addEventListener('change', (e) => {
                AppState.setTimeRange(e.target.value);
                ChartManager.update(AppState.currentMetric);
            });
        });

        // Month selector
        document.getElementById('month-select')?.addEventListener('change', (e) => {
            AppState.setMonth(e.target.value);
            ChartManager.update(AppState.currentMetric);
        });

        // Year selector
        document.getElementById('year-select')?.addEventListener('change', (e) => {
            AppState.setYear(e.target.value);
            ChartManager.update(AppState.currentMetric);
        });

        // CSV Export
        document.getElementById('export-csv')?.addEventListener('click', () => {
            CSVExporter.export();
        });
    },

    handleMetricChange(metric) {
        // Update active tab
        document.querySelectorAll('#data-tabs .nav-link').forEach(tab => {
            tab.classList.remove('active');
            if (tab.getAttribute('data-metric') === metric) {
                tab.classList.add('active');
            }
        });

        AppState.setMetric(metric);
        ChartManager.update(metric);
    }
};

// ============================================================================
// Year Selector Population
// ============================================================================
function populateYearSelector() {
    const yearSelect = document.getElementById('year-select');
    if (!yearSelect) return;

    const currentYear = new Date().getFullYear();
    const startYear = 2023; // Start from 2023

    // Add "Current" option
    const currentOption = document.createElement('option');
    currentOption.value = '';
    currentOption.textContent = 'Текущий';
    yearSelect.appendChild(currentOption);

    // Add years from start to current
    for (let year = currentYear; year >= startYear; year--) {
        const option = document.createElement('option');
        option.value = year;
        option.textContent = year;
        if (year === currentYear) {
            option.selected = true;
        }
        yearSelect.appendChild(option);
    }
}

// ============================================================================
// Auto-refresh
// ============================================================================
function startAutoRefresh() {
    // Initial fetch
    DataManager.fetchData();

    // Set up periodic refresh
    AppState.refreshTimer = setInterval(() => {
        DataManager.fetchData();
    }, CONFIG.refreshInterval);
}

function stopAutoRefresh() {
    if (AppState.refreshTimer) {
        clearInterval(AppState.refreshTimer);
        AppState.refreshTimer = null;
    }
}

// ============================================================================
// Debug Utilities
// ============================================================================
window.UgAgroDebug = {
    clearAllData() {
        if (confirm('Удалить все сохраненные данные? Это действие нельзя отменить.')) {
            DataStorage.clearAll();
            console.log('All data cleared');
            location.reload();
        }
    },

    clearMetric(metric) {
        const config = METRICS[metric];
        if (config) {
            DataStorage.clear(config.storageKey);
            console.log(`Cleared data for ${metric}`);
            ChartManager.update(AppState.currentMetric);
        } else {
            console.error('Unknown metric:', metric);
        }
    },

    showStorage() {
        console.log('=== LocalStorage Contents ===');
        Object.keys(METRICS).forEach(metric => {
            const config = METRICS[metric];
            const data = DataStorage.load(config.storageKey);
            console.log(`${metric}:`, data ? data.length + ' items' : 'empty');
        });
    },

    exportAllData() {
        const allData = {};
        Object.keys(METRICS).forEach(metric => {
            const config = METRICS[metric];
            allData[metric] = DataStorage.load(config.storageKey);
        });
        console.log(JSON.stringify(allData, null, 2));
        return allData;
    },

    simulateData(metric, count = 50) {
        const config = METRICS[metric];
        if (!config) {
            console.error('Unknown metric:', metric);
            return;
        }

        const now = Date.now();
        const data = [];
        for (let i = 0; i < count; i++) {
            const timestamp = new Date(now - (count - i) * 60000).toISOString(); // 1 minute intervals
            const value = Math.random() * (config.thresholds.max - config.thresholds.min) + config.thresholds.min;
            data.push({ timestamp, value: parseFloat(value.toFixed(2)) });
        }

        DataStorage.save(config.storageKey, data);
        console.log(`Generated ${count} data points for ${metric}`);
        ChartManager.update(AppState.currentMetric);
    }
};

// ============================================================================
// Application Initialization
// ============================================================================
document.addEventListener('DOMContentLoaded', () => {
    console.log('UgAgro Dashboard initializing...');

    // Initialize components
    AppState.init();
    ThemeManager.init();
    ChartManager.init();
    EventManager.init();
    populateYearSelector();

    // Load saved state
    const savedMetric = AppState.currentMetric;
    const savedTimeRange = AppState.timeRange;

    // Set active tab
    document.querySelectorAll('#data-tabs .nav-link').forEach(tab => {
        if (tab.getAttribute('data-metric') === savedMetric) {
            tab.classList.add('active');
        } else {
            tab.classList.remove('active');
        }
    });

    // Set active time range
    document.getElementById(`range-${savedTimeRange === 1 ? 'hour' : savedTimeRange === 24 ? 'day' : savedTimeRange === 72 ? '3days' : 'week'}`)?.click();

    // Update chart with saved data
    ChartManager.update(savedMetric);

    // Start auto-refresh
    startAutoRefresh();

    console.log('UgAgro Dashboard initialized successfully');
    console.log('Debug utilities available at window.UgAgroDebug');
});

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    stopAutoRefresh();
});
