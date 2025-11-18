-- =====================================================
-- UGAGRO GREENHOUSE DATABASE SCHEMA
-- =====================================================
-- Инициализация базы данных для n8n workflows
-- Автоматически создает все необходимые таблицы
-- =====================================================

-- Таблица показаний датчиков
CREATE TABLE IF NOT EXISTS ugagro_readings (
    id SERIAL PRIMARY KEY,

    -- Временные метки
    timestamp INTEGER NOT NULL,
    timestamp_iso TIMESTAMP NOT NULL DEFAULT NOW(),
    received_at TIMESTAMP NOT NULL DEFAULT NOW(),

    -- Основные датчики (обязательные)
    temperature DECIMAL(5,2) NOT NULL,
    humidity DECIMAL(5,2) NOT NULL,
    water_level DECIMAL(5,2) NOT NULL,
    wind_speed DECIMAL(5,2) NOT NULL,

    -- Дополнительные датчики
    pyrano DECIMAL(5,2) DEFAULT 0,
    solution_temperature DECIMAL(5,2),
    outdoor_temperature DECIMAL(5,2),
    outdoor_humidity DECIMAL(5,2),

    -- Датчики мата
    mat_temperature DECIMAL(5,2),
    mat_hum DECIMAL(5,2),
    mat_ec DECIMAL(5,2),
    mat_ph DECIMAL(5,2),

    -- Состояния устройств
    window_position INTEGER DEFAULT 0,
    fan_state BOOLEAN DEFAULT FALSE,
    heat_state BOOLEAN DEFAULT FALSE,
    pump_state BOOLEAN DEFAULT FALSE,
    fog_state BOOLEAN DEFAULT FALSE,
    sol_heat_state BOOLEAN DEFAULT FALSE,
    hydro_mix BOOLEAN DEFAULT FALSE,
    fill_state BOOLEAN DEFAULT FALSE,

    -- Режимы и счетчики
    watering_mode INTEGER DEFAULT 0,
    rad_sum DECIMAL(10,2) DEFAULT 0,
    cycle_count INTEGER DEFAULT 0,
    last_watering_start_unix INTEGER,

    -- Принудительный полив
    force_watering_active BOOLEAN DEFAULT FALSE,
    force_watering_override BOOLEAN DEFAULT FALSE,
    force_watering_end_time_unix INTEGER,

    -- Индексы для быстрого поиска
    CONSTRAINT unique_timestamp UNIQUE (timestamp)
);

-- Индексы для оптимизации запросов
CREATE INDEX IF NOT EXISTS idx_readings_timestamp ON ugagro_readings(timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_readings_timestamp_iso ON ugagro_readings(timestamp_iso DESC);
CREATE INDEX IF NOT EXISTS idx_readings_received_at ON ugagro_readings(received_at DESC);

-- =====================================================

-- Таблица состояний оповещений Telegram
CREATE TABLE IF NOT EXISTS telegram_alert_states (
    id SERIAL PRIMARY KEY,
    alert_type VARCHAR(50) NOT NULL UNIQUE,
    is_active BOOLEAN DEFAULT FALSE,
    last_sent_at TIMESTAMP,
    acknowledged_at TIMESTAMP,
    mute_until TIMESTAMP,
    alert_count INTEGER DEFAULT 0,
    current_value DECIMAL(10,2),
    threshold_value DECIMAL(10,2),
    updated_at TIMESTAMP NOT NULL DEFAULT NOW()
);

-- Предзаполнение типов оповещений
INSERT INTO telegram_alert_states (alert_type, is_active) VALUES
    ('high_temperature', FALSE),
    ('low_temperature', FALSE),
    ('high_humidity', FALSE),
    ('low_humidity', FALSE),
    ('low_water_level', FALSE),
    ('high_wind_speed', FALSE)
ON CONFLICT (alert_type) DO NOTHING;

-- =====================================================

-- Таблица истории оповещений
CREATE TABLE IF NOT EXISTS ugagro_alerts_history (
    id SERIAL PRIMARY KEY,
    alert_type VARCHAR(50) NOT NULL,
    alert_message TEXT NOT NULL,
    sensor_value DECIMAL(10,2),
    threshold_value DECIMAL(10,2),
    severity VARCHAR(20) DEFAULT 'warning',
    acknowledged BOOLEAN DEFAULT FALSE,
    acknowledged_at TIMESTAMP,
    created_at TIMESTAMP NOT NULL DEFAULT NOW(),
    resolved_at TIMESTAMP
);

-- Индексы для истории оповещений
CREATE INDEX IF NOT EXISTS idx_alerts_created_at ON ugagro_alerts_history(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alerts_type ON ugagro_alerts_history(alert_type);
CREATE INDEX IF NOT EXISTS idx_alerts_acknowledged ON ugagro_alerts_history(acknowledged);

-- =====================================================

-- Функция автоматической очистки старых данных
CREATE OR REPLACE FUNCTION cleanup_old_data() RETURNS void AS $$
BEGIN
    -- Удаление показаний старше 7 дней
    DELETE FROM ugagro_readings
    WHERE timestamp < EXTRACT(EPOCH FROM NOW() - INTERVAL '7 days');

    -- Удаление истории оповещений старше 30 дней
    DELETE FROM ugagro_alerts_history
    WHERE created_at < NOW() - INTERVAL '30 days';

    RAISE NOTICE 'Cleanup completed: old readings and alerts removed';
END;
$$ LANGUAGE plpgsql;

-- =====================================================

-- Представление для последних показаний (для быстрого доступа)
CREATE OR REPLACE VIEW latest_readings AS
SELECT * FROM ugagro_readings
ORDER BY timestamp DESC
LIMIT 100;

-- =====================================================

-- Представление для активных оповещений
CREATE OR REPLACE VIEW active_alerts AS
SELECT
    a.*,
    CASE
        WHEN a.last_sent_at IS NULL THEN 'never_sent'
        WHEN a.acknowledged_at IS NULL AND a.is_active THEN 'active'
        WHEN a.acknowledged_at > a.last_sent_at THEN 'acknowledged'
        ELSE 'inactive'
    END as status
FROM telegram_alert_states a
WHERE a.is_active = TRUE;

-- =====================================================

-- Вывод статистики
DO $$
BEGIN
    RAISE NOTICE '==============================================';
    RAISE NOTICE 'UgAgro Greenhouse Database initialized!';
    RAISE NOTICE '==============================================';
    RAISE NOTICE 'Tables created:';
    RAISE NOTICE '  - ugagro_readings (sensor data storage)';
    RAISE NOTICE '  - telegram_alert_states (alert states)';
    RAISE NOTICE '  - ugagro_alerts_history (alert history)';
    RAISE NOTICE '';
    RAISE NOTICE 'Views created:';
    RAISE NOTICE '  - latest_readings';
    RAISE NOTICE '  - active_alerts';
    RAISE NOTICE '';
    RAISE NOTICE 'Functions created:';
    RAISE NOTICE '  - cleanup_old_data()';
    RAISE NOTICE '==============================================';
END $$;
