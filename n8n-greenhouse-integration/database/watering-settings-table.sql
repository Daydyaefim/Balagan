-- =====================================================
-- Таблица настроек полива
-- =====================================================

CREATE TABLE IF NOT EXISTS watering_settings (
    id SERIAL PRIMARY KEY,

    -- Общие настройки
    rad_threshold DECIMAL(10,2) DEFAULT 500.0,     -- Порог радиации для режима 2
    watering_duration_sec INTEGER DEFAULT 60,       -- Длительность полива в секундах
    max_interval_minutes INTEGER DEFAULT 180,       -- Максимальный интервал между поливами (3 часа)

    -- Окно поливов
    watering_window_start TIME DEFAULT '08:00:00',  -- Начало окна поливов
    watering_window_end TIME DEFAULT '20:00:00',    -- Конец окна поливов

    -- Настройки для ручного режима (mode 0)
    manual_interval_minutes INTEGER DEFAULT 60,     -- Интервал между поливами в ручном режиме

    -- Настройки для утреннего режима (mode 1)
    morning_count INTEGER DEFAULT 3,                -- Количество утренних поливов
    morning_start_time TIME DEFAULT '08:00:00',     -- Время первого утреннего полива
    morning_interval_minutes INTEGER DEFAULT 30,    -- Интервал между утренними поливами

    updated_at TIMESTAMP NOT NULL DEFAULT NOW()
);

-- Вставить значения по умолчанию
INSERT INTO watering_settings (
    rad_threshold,
    watering_duration_sec,
    max_interval_minutes,
    watering_window_start,
    watering_window_end,
    manual_interval_minutes,
    morning_count,
    morning_start_time,
    morning_interval_minutes
) VALUES (
    500.0,      -- rad_threshold
    60,         -- watering_duration_sec
    180,        -- max_interval_minutes (3 часа)
    '08:00:00', -- watering_window_start
    '20:00:00', -- watering_window_end
    60,         -- manual_interval_minutes (1 час)
    3,          -- morning_count
    '08:00:00', -- morning_start_time
    30          -- morning_interval_minutes
)
ON CONFLICT DO NOTHING;

-- Индекс для быстрого доступа
CREATE INDEX IF NOT EXISTS idx_watering_settings_updated_at
ON watering_settings(updated_at DESC);

-- =====================================================
