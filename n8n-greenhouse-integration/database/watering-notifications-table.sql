-- =====================================================
-- Таблица для отслеживания уведомлений о поливе
-- =====================================================

CREATE TABLE IF NOT EXISTS watering_notifications (
    id SERIAL PRIMARY KEY,

    -- Состояние
    last_pump_state BOOLEAN DEFAULT FALSE,
    last_check_timestamp TIMESTAMP NOT NULL DEFAULT NOW(),

    -- Статистика уведомлений
    last_notification_type VARCHAR(20),  -- 'start' или 'end'
    last_notification_sent_at TIMESTAMP,
    notifications_count INTEGER DEFAULT 0,

    -- Информация о последнем поливе
    last_watering_start TIMESTAMP,
    last_watering_end TIMESTAMP,
    last_watering_duration_sec INTEGER,

    updated_at TIMESTAMP NOT NULL DEFAULT NOW()
);

-- Вставить начальное значение
INSERT INTO watering_notifications (
    last_pump_state,
    last_check_timestamp
) VALUES (
    FALSE,
    NOW()
)
ON CONFLICT DO NOTHING;

-- =====================================================
