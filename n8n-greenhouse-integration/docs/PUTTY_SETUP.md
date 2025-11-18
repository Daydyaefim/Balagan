# 🔐 НАСТРОЙКА POSTGRESQL ЧЕРЕЗ PUTTY (SSH)

## 📋 ЧТО ПОНАДОБИТСЯ

- ✅ PuTTY (скачать: https://www.putty.org/)
- ✅ SSH доступ к VPS Beget (IP, логин, пароль)
- ✅ Файл `init-greenhouse-db.sql` скачанный с GitHub

---

## ВАРИАНТ 1: ЧЕРЕЗ SSH + DOCKER (Рекомендуется)

### Шаг 1: Подключитесь к серверу через PuTTY

1. **Откройте PuTTY**

2. **Заполните данные подключения:**
   ```
   Host Name (or IP address): ВАШ_IP_АДРЕС_VPS
   Port: 22
   Connection type: SSH
   ```

3. **Нажмите "Open"**

4. **Первое подключение:**
   - Появится предупреждение о безопасности
   - Нажмите **"Yes"** (добавить ключ в кэш)

5. **Введите логин:**
   ```
   login as: ваш_логин
   ```

6. **Введите пароль:**
   ```
   password: ваш_пароль
   ```
   (пароль не отображается при вводе - это нормально)

7. **Вы подключились!** Увидите приглашение:
   ```
   [user@hostname ~]$
   ```

---

### Шаг 2: Найдите имя контейнера PostgreSQL

Выполните команду:
```bash
docker ps
```

Найдите строку с `postgres:11`:
```
CONTAINER ID   IMAGE          COMMAND                  CREATED       STATUS       PORTS                    NAMES
abc123def456   postgres:11    "docker-entrypoint.s…"   2 weeks ago   Up 5 days    5432/tcp                 n8n-postgres-1
```

**Запомните имя контейнера** (последняя колонка), например: `n8n-postgres-1`

---

### Шаг 3: Загрузите SQL скрипт на сервер

#### Способ A: Через SCP (WinSCP)

1. **Скачайте WinSCP:** https://winscp.net/
2. Подключитесь к VPS (тот же IP/логин/пароль)
3. Найдите папку `/opt/beget/n8n/`
4. Загрузите файл `init-greenhouse-db.sql`

#### Способ B: Через nano (прямо в PuTTY)

```bash
# Перейдите в папку
cd /opt/beget/n8n/

# Создайте файл
nano init-greenhouse-db.sql
```

**Скопируйте содержимое SQL скрипта из GitHub:**
```
https://github.com/Daydyaefim/Balagan/blob/claude/n8n-esp32-greenhouse-integration-0182vddx43zfdqyfMHo57bfV/n8n-greenhouse-integration/database/init-greenhouse-db.sql
```

1. Откройте ссылку в браузере
2. Нажмите **Raw**
3. **Ctrl+A** → **Ctrl+C** (скопировать весь текст)
4. В PuTTY кликните **правой кнопкой мыши** (вставится текст)
5. Нажмите **Ctrl+X** → **Y** → **Enter** (сохранить)

#### Способ C: Скачать через wget

```bash
cd /opt/beget/n8n/
wget -O init-greenhouse-db.sql "https://raw.githubusercontent.com/Daydyaefim/Balagan/claude/n8n-esp32-greenhouse-integration-0182vddx43zfdqyfMHo57bfV/n8n-greenhouse-integration/database/init-greenhouse-db.sql"
```

---

### Шаг 4: Выполните SQL скрипт

**Замените `n8n-postgres-1` на имя вашего контейнера из Шага 2!**

```bash
docker exec -i n8n-postgres-1 psql -U root -d n8n < /opt/beget/n8n/init-greenhouse-db.sql
```

**Вы должны увидеть:**
```
CREATE TABLE
CREATE INDEX
CREATE INDEX
...
NOTICE: ==============================================
NOTICE: UgAgro Greenhouse Database initialized!
NOTICE: ==============================================
NOTICE: Tables created:
NOTICE:   - ugagro_readings (sensor data storage)
NOTICE:   - telegram_alert_states (alert states)
NOTICE:   - ugagro_alerts_history (alert history)
...
```

✅ **База данных инициализирована!**

---

### Шаг 5: Проверьте что таблицы созданы

```bash
docker exec -it n8n-postgres-1 psql -U root -d n8n -c "SELECT tablename FROM pg_tables WHERE schemaname='public';"
```

**Должны увидеть:**
```
        tablename
--------------------------
 ugagro_readings
 telegram_alert_states
 ugagro_alerts_history
(3 rows)
```

✅ **Всё работает!**

---

## ВАРИАНТ 2: ИНТЕРАКТИВНЫЙ РЕЖИМ (если способ 1 не работает)

### Подключитесь к PostgreSQL интерактивно

```bash
docker exec -it n8n-postgres-1 psql -U root -d n8n
```

Увидите приглашение:
```
n8n=#
```

### Скопируйте и вставьте SQL команды

1. **Откройте SQL скрипт на GitHub в браузере**
2. **Скопируйте ВСЁ содержимое** (Ctrl+A → Ctrl+C)
3. **В PuTTY кликните правой кнопкой мыши** (вставится текст)
4. **Нажмите Enter**

SQL команды выполнятся по очереди.

### Выйдите из PostgreSQL

```sql
\q
```

---

## 🧪 ПРОВЕРКА РАБОТЫ

### 1. Проверьте количество записей

```bash
docker exec -it n8n-postgres-1 psql -U root -d n8n -c "SELECT COUNT(*) FROM ugagro_readings;"
```

**Должно быть:**
```
 count
-------
     0
(1 row)
```

(Пока 0 - это нормально, данные появятся когда ESP32 начнет отправлять)

### 2. Проверьте состояния оповещений

```bash
docker exec -it n8n-postgres-1 psql -U root -d n8n -c "SELECT alert_type, is_active FROM telegram_alert_states;"
```

**Должны увидеть:**
```
     alert_type     | is_active
--------------------+-----------
 high_temperature   | f
 low_temperature    | f
 high_humidity      | f
 low_humidity       | f
 low_water_level    | f
 high_wind_speed    | f
(6 rows)
```

✅ **База готова к работе!**

---

## 🔍 ПОЛЕЗНЫЕ КОМАНДЫ

### Просмотр последних данных

```bash
docker exec -it n8n-postgres-1 psql -U root -d n8n -c "SELECT timestamp_iso, temperature, humidity FROM ugagro_readings ORDER BY timestamp DESC LIMIT 5;"
```

### Просмотр структуры таблицы

```bash
docker exec -it n8n-postgres-1 psql -U root -d n8n -c "\d ugagro_readings"
```

### Очистка всех данных (если нужно)

```bash
docker exec -it n8n-postgres-1 psql -U root -d n8n -c "TRUNCATE TABLE ugagro_readings CASCADE;"
```

### Удаление и пересоздание БД (полный сброс)

```bash
# Удалить таблицы
docker exec -it n8n-postgres-1 psql -U root -d n8n -c "DROP TABLE IF EXISTS ugagro_readings, telegram_alert_states, ugagro_alerts_history CASCADE;"

# Заново выполнить скрипт инициализации
docker exec -i n8n-postgres-1 psql -U root -d n8n < /opt/beget/n8n/init-greenhouse-db.sql
```

---

## ❓ ЧАСТЫЕ ПРОБЛЕМЫ

### "docker: command not found"

**Проблема:** Docker не установлен или не в PATH

**Решение:**
```bash
# Проверьте где Docker
which docker

# Если показывает путь (например /usr/bin/docker), используйте полный путь:
/usr/bin/docker exec -i n8n-postgres-1 psql ...
```

---

### "Error: No such container: n8n-postgres-1"

**Проблема:** Неверное имя контейнера

**Решение:**
```bash
# Посмотрите список контейнеров
docker ps

# Найдите точное имя контейнера с postgres:11
# Используйте его вместо n8n-postgres-1
```

---

### "permission denied"

**Проблема:** Нет прав на выполнение docker команд

**Решение:**
```bash
# Используйте sudo
sudo docker exec -i n8n-postgres-1 psql -U root -d n8n < /opt/beget/n8n/init-greenhouse-db.sql
```

Или добавьте пользователя в группу docker:
```bash
sudo usermod -aG docker $USER
# Затем перелогиньтесь (выйдите и зайдите заново)
```

---

### "psql: FATAL: password authentication failed"

**Проблема:** Неверный пароль в .env файле

**Решение:**
```bash
# Проверьте пароль в .env
cat /opt/beget/n8n/.env | grep POSTGRES_PASSWORD

# Используйте правильный пароль:
docker exec -i n8n-postgres-1 psql -U root -d n8n < /opt/beget/n8n/init-greenhouse-db.sql
```

---

### Файл не найден: "/opt/beget/n8n/init-greenhouse-db.sql"

**Проблема:** Файл находится в другой папке

**Решение:**
```bash
# Найдите файл
find /opt/beget -name "init-greenhouse-db.sql"

# Или используйте правильный путь
docker exec -i n8n-postgres-1 psql -U root -d n8n < /путь/к/файлу/init-greenhouse-db.sql
```

---

## 🎯 СЛЕДУЮЩИЕ ШАГИ

После успешной инициализации БД:

1. ✅ **Создайте Credential в n8n** (см. `POSTGRES_SETUP.md`)
2. ✅ **Импортируйте workflows** с суффиксом `-postgres.json`
3. ✅ **Настройте credentials в workflows**
4. ✅ **Активируйте workflows**
5. ✅ **Проверьте dashboard:** `https://ipedekdomus.beget.app/webhook/dashboard`

---

## 📚 ДОПОЛНИТЕЛЬНО

### Бэкап базы данных

```bash
docker exec n8n-postgres-1 pg_dump -U root -d n8n > /opt/beget/n8n/backup_$(date +%Y%m%d).sql
```

### Восстановление из бэкапа

```bash
docker exec -i n8n-postgres-1 psql -U root -d n8n < /opt/beget/n8n/backup_20250118.sql
```

---

## 💡 СОВЕТЫ ДЛЯ РАБОТЫ С PUTTY

### Копирование текста из PuTTY

- **Выделите текст мышью** → автоматически скопируется в буфер
- Вставьте в другом месте: **Ctrl+V**

### Вставка текста в PuTTY

- **Правая кнопка мыши** → вставляет из буфера обмена

### Сохранение сессии

1. В PuTTY главном окне заполните Host Name
2. В поле "Saved Sessions" введите имя (например "Beget VPS")
3. Нажмите **Save**
4. В следующий раз: выберите сессию → **Load** → **Open**

### Автологин (чтобы не вводить пароль каждый раз)

1. Connection → SSH → Auth
2. "Private key file": выберите ваш SSH ключ (.ppk файл)
3. Сохраните сессию

---

**Готово! Теперь база данных настроена и готова к работе!** 🚀
