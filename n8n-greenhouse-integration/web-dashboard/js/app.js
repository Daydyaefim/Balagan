// Конфигурация
const CONFIG = {
    API_URL: window.location.origin + '/webhook/api/readings',
    UPDATE_INTERVAL: 30000, // 30 секунд
    THRESHOLDS: {
        temperature: { min: 10, max: 40 },
        humidity: { min: 20, max: 85 },
        water_level: { min: 5 },
        wind_speed: { max: 11 }
    }
};

// Хранилище графиков
const charts = {};

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
    console.log('🌱 UgAgro Dashboard инициализирован');
    initCharts();
    fetchData();
    setInterval(fetchData, CONFIG.UPDATE_INTERVAL);
});

// Создание графиков
function initCharts() {
    const commonOptions = {
        responsive: true,
        maintainAspectRatio: true,
        plugins: {
            legend: {
                display: true,
                position: 'top'
            }
        },
        scales: {
            x: {
                display: true,
                title: {
                    display: true,
                    text: 'Время'
                },
                ticks: {
                    maxTicksLimit: 8
                }
            },
            y: {
                display: true,
                beginAtZero: false
            }
        }
    };

    // График температуры
    charts.temperature = new Chart(document.getElementById('chart-temperature'), {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Температура (°C)',
                data: [],
                borderColor: '#dc3545',
                backgroundColor: 'rgba(220, 53, 69, 0.1)',
                tension: 0.4,
                fill: true
            }, {
                label: 'Температура улицы (°C)',
                data: [],
                borderColor: '#007bff',
                backgroundColor: 'rgba(0, 123, 255, 0.1)',
                tension: 0.4,
                fill: false,
                borderDash: [5, 5]
            }]
        },
        options: commonOptions
    });

    // График влажности
    charts.humidity = new Chart(document.getElementById('chart-humidity'), {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Влажность (%)',
                data: [],
                borderColor: '#17a2b8',
                backgroundColor: 'rgba(23, 162, 184, 0.1)',
                tension: 0.4,
                fill: true
            }, {
                label: 'Влажность улицы (%)',
                data: [],
                borderColor: '#6c757d',
                backgroundColor: 'rgba(108, 117, 125, 0.1)',
                tension: 0.4,
                fill: false,
                borderDash: [5, 5]
            }]
        },
        options: commonOptions
    });

    // График уровня воды
    charts.water = new Chart(document.getElementById('chart-water'), {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Уровень воды (%)',
                data: [],
                borderColor: '#007bff',
                backgroundColor: 'rgba(0, 123, 255, 0.1)',
                tension: 0.4,
                fill: true
            }]
        },
        options: {
            ...commonOptions,
            scales: {
                ...commonOptions.scales,
                y: {
                    ...commonOptions.scales.y,
                    min: 0,
                    max: 100
                }
            }
        }
    });

    // График скорости ветра
    charts.wind = new Chart(document.getElementById('chart-wind'), {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Скорость ветра (м/с)',
                data: [],
                borderColor: '#28a745',
                backgroundColor: 'rgba(40, 167, 69, 0.1)',
                tension: 0.4,
                fill: true
            }]
        },
        options: {
            ...commonOptions,
            scales: {
                ...commonOptions.scales,
                y: {
                    ...commonOptions.scales.y,
                    min: 0
                }
            }
        }
    });
}

// Загрузка данных из API
async function fetchData() {
    try {
        updateConnectionStatus('loading');

        const response = await fetch(`${CONFIG.API_URL}?hours=24`);

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const result = await response.json();

        if (result.success && result.data) {
            updateDashboard(result.data);
            updateConnectionStatus('connected');
        } else {
            throw new Error('Некорректный формат данных');
        }
    } catch (error) {
        console.error('Ошибка загрузки данных:', error);
        updateConnectionStatus('error');
    }
}

// Обновление дашборда
function updateDashboard(data) {
    if (!data.latest) {
        console.warn('Нет последних данных');
        return;
    }

    const latest = data.latest;

    // Обновление текущих показаний
    updateSensorCard('temperature', latest.temperature, '°C');
    updateSensorCard('humidity', latest.humidity, '%');
    updateSensorCard('water', latest.water_level, '%');
    updateSensorCard('wind', latest.wind_speed, 'м/с');

    // Дополнительная информация
    document.getElementById('outdoor-temp').textContent = formatValue(latest.outdoor_temperature, 1);
    document.getElementById('outdoor-hum').textContent = formatValue(latest.outdoor_humidity, 1);
    document.getElementById('sol-temp').textContent = formatValue(latest.solution_temperature, 1);
    document.getElementById('pyrano-value').textContent = formatValue(latest.pyrano, 0);
    document.getElementById('window-pos').textContent = latest.window_position || '--';

    // Состояния оборудования
    updateEquipmentState('fan-state', latest.fan_state);
    updateEquipmentState('heat-state', latest.heat_state);
    updateEquipmentState('pump-state', latest.pump_state);

    // Обновление графиков
    if (data.chartData) {
        updateChart('temperature', data.chartData);
        updateChart('humidity', data.chartData);
        updateChart('water', data.chartData);
        updateChart('wind', data.chartData);
    }

    // Время последнего обновления
    document.getElementById('last-update').textContent = new Date().toLocaleString('ru-RU');
}

// Обновление карточки сенсора
function updateSensorCard(type, value, unit) {
    const valueElement = document.getElementById(`${type}-value`);
    const cardElement = document.getElementById(`card-${type}`);

    if (!valueElement || !cardElement) return;

    // Обновление значения
    valueElement.textContent = formatValue(value, 1);

    // Анимация обновления
    cardElement.classList.add('updating');
    setTimeout(() => cardElement.classList.remove('updating'), 500);

    // Определение статуса
    let status = 'normal';
    const thresholds = CONFIG.THRESHOLDS;

    if (type === 'temperature') {
        if (value < thresholds.temperature.min || value > thresholds.temperature.max) {
            status = 'critical';
        }
    } else if (type === 'humidity') {
        if (value < thresholds.humidity.min || value > thresholds.humidity.max) {
            status = 'critical';
        }
    } else if (type === 'water') {
        if (value < thresholds.water_level.min) {
            status = 'critical';
        } else if (value < 15) {
            status = 'warning';
        }
    } else if (type === 'wind') {
        if (value > thresholds.wind_speed.max) {
            status = 'critical';
        } else if (value > 8) {
            status = 'warning';
        }
    }

    // Применение стиля
    cardElement.className = `card sensor-card status-${status}`;
}

// Обновление состояния оборудования
function updateEquipmentState(elementId, state) {
    const element = document.getElementById(elementId);
    if (!element) return;

    const isOn = state === true || state === 1 || state === 'on';
    element.textContent = isOn ? 'ВКЛ' : 'ВЫКЛ';
    element.className = `badge ${isOn ? 'state-on' : 'state-off'}`;
}

// Обновление графика
function updateChart(chartName, chartData) {
    const chart = charts[chartName];
    if (!chart || !chartData) return;

    // Форматирование меток времени
    const labels = chartData.labels.map(label => {
        const date = new Date(label);
        return date.toLocaleTimeString('ru-RU', {
            hour: '2-digit',
            minute: '2-digit'
        });
    });

    if (chartName === 'temperature') {
        chart.data.labels = labels;
        chart.data.datasets[0].data = chartData.temperature;
        chart.data.datasets[1].data = chartData.outdoor_temperature;
    } else if (chartName === 'humidity') {
        chart.data.labels = labels;
        chart.data.datasets[0].data = chartData.humidity;
        chart.data.datasets[1].data = chartData.outdoor_humidity;
    } else if (chartName === 'water') {
        chart.data.labels = labels;
        chart.data.datasets[0].data = chartData.water_level;
    } else if (chartName === 'wind') {
        chart.data.labels = labels;
        chart.data.datasets[0].data = chartData.wind_speed;
    }

    chart.update('none'); // Обновление без анимации для плавности
}

// Обновление статуса подключения
function updateConnectionStatus(status) {
    const statusElement = document.getElementById('connection-status');
    if (!statusElement) return;

    if (status === 'connected') {
        statusElement.innerHTML = '<i class="bi bi-circle-fill text-success"></i> Подключено';
    } else if (status === 'loading') {
        statusElement.innerHTML = '<i class="bi bi-circle-fill text-warning"></i> Обновление...';
    } else if (status === 'error') {
        statusElement.innerHTML = '<i class="bi bi-circle-fill text-danger"></i> Ошибка';
    }
}

// Форматирование значения
function formatValue(value, decimals = 1) {
    if (value === null || value === undefined || isNaN(value)) {
        return '--';
    }
    return Number(value).toFixed(decimals);
}

// Экспорт данных в CSV (будущая функциональность)
function exportToCSV() {
    console.log('Экспорт в CSV...');
    // TODO: Реализовать экспорт
}
