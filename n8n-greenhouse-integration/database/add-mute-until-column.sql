-- =====================================================
-- Миграция: Добавление колонки mute_until
-- =====================================================
-- Добавляет поддержку временного отключения оповещений

-- Добавить колонку mute_until если её нет
ALTER TABLE telegram_alert_states
ADD COLUMN IF NOT EXISTS mute_until TIMESTAMP;

-- Проверка результата
DO $$
BEGIN
    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_name = 'telegram_alert_states'
        AND column_name = 'mute_until'
    ) THEN
        RAISE NOTICE 'SUCCESS: Column mute_until added to telegram_alert_states';
    ELSE
        RAISE EXCEPTION 'FAILED: Column mute_until not found';
    END IF;
END $$;
