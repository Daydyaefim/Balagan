-- =====================================================
-- Обновление настроек полива до корректных значений
-- =====================================================

-- Обновить существующие значения на корректные
UPDATE watering_settings
SET
    rad_threshold = 1.1,                     -- Было 500.0
    watering_duration_sec = 200,              -- Было 60
    max_interval_minutes = 90,                -- Было 180
    watering_window_start = '07:40:00',       -- Было '08:00:00'
    watering_window_end = '17:20:00',         -- Было '20:00:00'
    morning_start_time = '07:40:00',          -- Было '08:00:00'
    updated_at = NOW()
WHERE id = 1;

-- Проверить результат
SELECT
    'Обновлено успешно' as status,
    rad_threshold,
    watering_duration_sec,
    max_interval_minutes,
    watering_window_start,
    watering_window_end,
    morning_start_time,
    updated_at
FROM watering_settings
WHERE id = 1;

-- =====================================================
