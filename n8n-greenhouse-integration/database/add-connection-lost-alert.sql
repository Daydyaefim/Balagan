-- =====================================================
-- Добавить новый тип оповещения - потеря связи с ESP32
-- =====================================================

INSERT INTO telegram_alert_states (alert_type, is_active) VALUES
    ('esp32_connection_lost', FALSE)
ON CONFLICT (alert_type) DO NOTHING;

-- =====================================================
