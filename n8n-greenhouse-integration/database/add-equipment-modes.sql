-- =====================================================
-- Add Equipment Mode Columns
-- =====================================================
-- Добавление столбцов для режимов работы оборудования
-- Режимы: auto, manual, forced
-- =====================================================

-- Добавляем столбцы для режимов оборудования
ALTER TABLE ugagro_readings
    ADD COLUMN IF NOT EXISTS fan_mode VARCHAR(20) DEFAULT 'auto',
    ADD COLUMN IF NOT EXISTS heat_mode VARCHAR(20) DEFAULT 'auto',
    ADD COLUMN IF NOT EXISTS pump_mode VARCHAR(20) DEFAULT 'auto',
    ADD COLUMN IF NOT EXISTS fog_mode VARCHAR(20) DEFAULT 'auto',
    ADD COLUMN IF NOT EXISTS hydro_mix_mode VARCHAR(20) DEFAULT 'auto';

-- Добавляем столбец для солнечной радиации (алиас для pyrano)
ALTER TABLE ugagro_readings
    ADD COLUMN IF NOT EXISTS solar_radiation DECIMAL(5,2);

-- Создаем триггер для синхронизации solar_radiation и pyrano
CREATE OR REPLACE FUNCTION sync_solar_radiation() RETURNS TRIGGER AS $$
BEGIN
    IF NEW.pyrano IS NOT NULL THEN
        NEW.solar_radiation := NEW.pyrano;
    END IF;
    IF NEW.solar_radiation IS NOT NULL THEN
        NEW.pyrano := NEW.solar_radiation;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trigger_sync_solar_radiation ON ugagro_readings;
CREATE TRIGGER trigger_sync_solar_radiation
    BEFORE INSERT OR UPDATE ON ugagro_readings
    FOR EACH ROW
    EXECUTE FUNCTION sync_solar_radiation();

-- Обновляем существующие записи
UPDATE ugagro_readings SET solar_radiation = pyrano WHERE solar_radiation IS NULL;

-- Вывод статистики
DO $$
BEGIN
    RAISE NOTICE '==============================================';
    RAISE NOTICE 'Equipment modes migration completed!';
    RAISE NOTICE '==============================================';
    RAISE NOTICE 'Added columns:';
    RAISE NOTICE '  - fan_mode (VARCHAR)';
    RAISE NOTICE '  - heat_mode (VARCHAR)';
    RAISE NOTICE '  - pump_mode (VARCHAR)';
    RAISE NOTICE '  - fog_mode (VARCHAR)';
    RAISE NOTICE '  - hydro_mix_mode (VARCHAR)';
    RAISE NOTICE '  - solar_radiation (synced with pyrano)';
    RAISE NOTICE '==============================================';
END $$;
