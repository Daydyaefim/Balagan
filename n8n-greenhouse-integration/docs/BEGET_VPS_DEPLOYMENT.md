# 🚀 Развертывание на Beget VPS

## Полное руководство по настройке n8n интеграции на VPS от Beget

---

## 📋 Предварительные требования

- ✅ VPS сервер от Beget (любой тариф)
- ✅ n8n установленная через маркетплейс Beget
- ✅ SSH доступ к серверу
- ✅ Базовые знания Linux

---

## 🎯 Архитектура на Beget VPS

```
Beget VPS Server
├── n8n (установлена через маркетплейс)
│   ├── Порт: 5678
│   └── Workflows для обработки MQTT
├── Nginx (для веб-дашборда)
│   ├── Порт: 80/443
│   └── Статические файлы дашборда
├── MQTT Client (в n8n)
│   └── Подключение к broker.emqx.io
└── Данные
    └── /home/username/greenhouse-data/
```

---

## 📦 Шаг 1: Установка n8n через Beget Marketplace

### 1.1 Через панель управления Beget

1. Войдите в панель управления Beget
2. Перейдите в раздел **"VPS"** → **"Marketplace"**
3. Найдите **"n8n"** в списке приложений
4. Нажмите **"Установить"**
5. Дождитесь завершения установки

### 1.2 Доступ к n8n

После установки n8n будет доступна по адресу:
```
http://ваш-ip:5678
```

Или если настроен домен:
```
https://n8n.ваш-домен.ru
```

**Важно:** Запомните логин и пароль, которые были созданы при установке!

---

## 🔐 Шаг 2: Подключение по SSH

### 2.1 Получение SSH доступа

В панели Beget:
1. **VPS** → **Настройки**
2. Найдите данные для SSH:
   - IP адрес
   - Порт SSH (обычно 22)
   - Логин
   - Пароль

### 2.2 Подключение

**Linux/Mac:**
```bash
ssh username@your-vps-ip -p 22
```

**Windows (PuTTY):**
- Host: `your-vps-ip`
- Port: `22`
- Username: `username`
- Password: `ваш_пароль`

---

## 📂 Шаг 3: Подготовка директорий для данных

```bash
# Подключитесь по SSH
ssh username@your-vps-ip

# Создайте директорию для данных теплицы
mkdir -p ~/greenhouse-data
chmod 755 ~/greenhouse-data

# Создайте файлы для хранения данных
cd ~/greenhouse-data
touch ugagro_readings.json
touch telegram_alert_states.json
touch ugagro_alerts_history.json
touch cleanup_log.json

# Установите права
chmod 666 *.json

# Проверьте структуру
ls -la
```

Должно получиться:
```
/home/username/greenhouse-data/
├── ugagro_readings.json
├── telegram_alert_states.json
├── ugagro_alerts_history.json
└── cleanup_log.json
```

---

## 🔧 Шаг 4: Настройка переменных окружения n8n

### 4.1 Найти конфигурационный файл n8n

```bash
# Найдите процесс n8n
ps aux | grep n8n

# Обычно конфиг находится в
nano ~/.n8n/.env
```

### 4.2 Добавить переменные окружения

Если файл `.env` не существует, создайте его:

```bash
nano ~/.n8n/.env
```

Добавьте следующие строки:

```env
# Telegram Bot
TELEGRAM_BOT_TOKEN=ваш_токен_от_BotFather
TELEGRAM_CHAT_ID=ваш_chat_id

# Timezone
GENERIC_TIMEZONE=Europe/Moscow
TZ=Europe/Moscow

# Data paths
GREENHOUSE_DATA_DIR=/home/username/greenhouse-data
```

**Замените:**
- `username` на ваш логин в системе
- `ваш_токен_от_BotFather` на реальный токен
- `ваш_chat_id` на ваш ID в Telegram

Сохраните: `Ctrl+O`, `Enter`, `Ctrl+X`

### 4.3 Перезапустить n8n

```bash
# Найдите PID процесса n8n
ps aux | grep n8n

# Перезапустите (команда зависит от способа установки)
# Вариант 1: systemd
sudo systemctl restart n8n

# Вариант 2: pm2
pm2 restart n8n

# Вариант 3: если установлен через Docker
docker restart n8n
```

---

## 📥 Шаг 5: Загрузка workflows на сервер

### 5.1 Скачать репозиторий на VPS

```bash
# Перейдите в домашнюю директорию
cd ~

# Клонируйте репозиторий
git clone https://github.com/Daydyaefim/Balagan.git

# Перейдите в папку с интеграцией
cd Balagan/n8n-greenhouse-integration

# Проверьте наличие workflows
ls -la workflows/
```

### 5.2 Импорт workflows в n8n UI

1. Откройте браузер: `http://ваш-ip:5678`
2. Войдите с логином и паролем
3. Перейдите: **Workflows** → **Import from File**
4. Импортируйте все 5 workflows:
   - `01-mqtt-data-pipeline.json`
   - `02-critical-alerts-monitor.json`
   - `03-telegram-callback-handler.json`
   - `04-web-dashboard-api.json`
   - `05-data-rotation-cleanup.json`

---

## ⚙️ Шаг 6: Настройка Credentials в n8n

### 6.1 MQTT Broker

1. В n8n UI: **Settings** → **Credentials** → **New**
2. Выберите тип: **MQTT**
3. Заполните:
   - **Protocol**: `mqtt`
   - **Host**: `broker.emqx.io`
   - **Port**: `1883`
   - **Username**: (оставить пустым)
   - **Password**: (оставить пустым)
4. Нажмите **Save**

### 6.2 Telegram Bot API

1. **Settings** → **Credentials** → **New**
2. Выберите тип: **Telegram API**
3. Заполните:
   - **Access Token**: вставьте токен из `.env`
4. Нажмите **Save**

---

## 🔗 Шаг 7: Применение Credentials к Workflows

Для **каждого** из 5 workflows:

1. Откройте workflow
2. Найдите ноды с иконкой ключа (требуют credentials):
   - **MQTT Trigger** / **MQTT** → выберите созданный MQTT credential
   - **Telegram** / **Telegram Trigger** → выберите Telegram credential
3. **Обновите пути к файлам** в Code нодах:
   - Замените `/data/` на `/home/username/greenhouse-data/`
4. Нажмите **Save**

### Пример изменения пути в Code ноде:

**Было:**
```javascript
operation: "read",
filePath: "/data/ugagro_readings.json"
```

**Стало:**
```javascript
operation: "read",
filePath: "/home/username/greenhouse-data/ugagro_readings.json"
```

**ВАЖНО:** Замените `username` на ваш фактический логин!

---

## ✅ Шаг 8: Активация Workflows

Для каждого workflow:
1. Откройте workflow
2. Переключите **Active** (в правом верхнем углу)
3. Убедитесь, что статус стал зеленым

**Порядок активации:**
1. ✅ 01 - MQTT Data Pipeline (первым!)
2. ✅ 02 - Critical Alerts Monitor
3. ✅ 03 - Telegram Callback Handler
4. ✅ 04 - Web Dashboard API
5. ✅ 05 - Data Rotation & Cleanup

---

## 🌐 Шаг 9: Развертывание Веб-Дашборда

### 9.1 Установка Nginx (если не установлен)

```bash
# Для Ubuntu/Debian
sudo apt update
sudo apt install nginx -y

# Для CentOS/RHEL
sudo yum install nginx -y

# Запустить и включить автозагрузку
sudo systemctl start nginx
sudo systemctl enable nginx
```

### 9.2 Копирование файлов дашборда

```bash
# Создайте директорию для дашборда
sudo mkdir -p /var/www/greenhouse-dashboard

# Скопируйте файлы
sudo cp -r ~/Balagan/n8n-greenhouse-integration/web-dashboard/* /var/www/greenhouse-dashboard/

# Установите права
sudo chown -R www-data:www-data /var/www/greenhouse-dashboard
sudo chmod -R 755 /var/www/greenhouse-dashboard
```

### 9.3 Настройка Nginx конфига

```bash
sudo nano /etc/nginx/sites-available/greenhouse
```

Вставьте следующую конфигурацию:

```nginx
server {
    listen 80;
    server_name ваш-домен.ru;  # Замените на ваш домен или IP

    root /var/www/greenhouse-dashboard;
    index index.html;

    # Логи
    access_log /var/log/nginx/greenhouse_access.log;
    error_log /var/log/nginx/greenhouse_error.log;

    # Главная страница
    location / {
        try_files $uri $uri/ /index.html;
    }

    # Проксирование API к n8n
    location /webhook/ {
        proxy_pass http://localhost:5678/webhook/;
        proxy_http_version 1.1;

        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }

    # CORS для API
    location ~* \.(json)$ {
        add_header Access-Control-Allow-Origin *;
    }

    # Кэширование статики
    location ~* \.(css|js|jpg|jpeg|png|gif|ico|svg)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }

    # Gzip
    gzip on;
    gzip_types text/plain text/css application/json application/javascript;
}
```

### 9.4 Активация конфига и перезагрузка Nginx

```bash
# Создать симлинк
sudo ln -s /etc/nginx/sites-available/greenhouse /etc/nginx/sites-enabled/

# Проверить конфигурацию
sudo nginx -t

# Перезагрузить Nginx
sudo systemctl reload nginx
```

### 9.5 Настройка Firewall (если включен)

```bash
# UFW (Ubuntu)
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw allow 5678/tcp

# Firewalld (CentOS)
sudo firewall-cmd --permanent --add-port=80/tcp
sudo firewall-cmd --permanent --add-port=443/tcp
sudo firewall-cmd --permanent --add-port=5678/tcp
sudo firewall-cmd --reload
```

---

## 🔒 Шаг 10: Настройка SSL (опционально, но рекомендуется)

### Установка Certbot

```bash
# Ubuntu/Debian
sudo apt install certbot python3-certbot-nginx -y

# CentOS/RHEL
sudo yum install certbot python3-certbot-nginx -y
```

### Получение SSL сертификата

```bash
sudo certbot --nginx -d ваш-домен.ru
```

Следуйте инструкциям. Certbot автоматически настроит HTTPS.

---

## ✅ Шаг 11: Проверка работоспособности

### 11.1 Проверка n8n

```bash
curl http://localhost:5678/healthcheck
```

Должен вернуть: `{"status":"ok"}`

### 11.2 Проверка Telegram бота

1. Откройте Telegram
2. Найдите вашего бота
3. Отправьте `/start`
4. Бот должен ответить

### 11.3 Проверка веб-дашборда

Откройте браузер:
```
http://ваш-домен.ru
```

или

```
http://ваш-ip
```

Должен открыться дашборд с карточками и графиками.

### 11.4 Проверка MQTT данных

В n8n:
1. Откройте **Workflow: 01 - MQTT Data Pipeline**
2. Нажмите **Execute Workflow**
3. Через ~30 секунд (когда ESP32 отправит данные) должна появиться новая запись

Проверить файл данных:
```bash
cat ~/greenhouse-data/ugagro_readings.json | jq . | head -20
```

---

## 🔍 Шаг 12: Логи и Мониторинг

### n8n логи

```bash
# Если через systemd
sudo journalctl -u n8n -f

# Если через pm2
pm2 logs n8n

# Если через Docker
docker logs n8n -f
```

### Nginx логи

```bash
# Access log
sudo tail -f /var/log/nginx/greenhouse_access.log

# Error log
sudo tail -f /var/log/nginx/greenhouse_error.log
```

### Данные теплицы

```bash
# Последние показания
tail -20 ~/greenhouse-data/ugagro_readings.json | jq .

# Состояния оповещений
cat ~/greenhouse-data/telegram_alert_states.json | jq .

# История оповещений
tail -10 ~/greenhouse-data/ugagro_alerts_history.json | jq .
```

---

## 🛠️ Устранение Неполадок

### n8n не запускается

```bash
# Проверить статус
sudo systemctl status n8n

# Перезапустить
sudo systemctl restart n8n

# Посмотреть логи
sudo journalctl -u n8n -n 50
```

### Workflow не активируется

1. Проверьте credentials
2. Убедитесь, что пути к файлам корректны
3. Проверьте логи n8n

### Telegram бот не отвечает

1. Проверьте `TELEGRAM_BOT_TOKEN` в `.env`
2. Убедитесь, что написали боту `/start`
3. Проверьте workflow #2 активен
4. Проверьте логи n8n

### Дашборд не загружается

```bash
# Проверить Nginx
sudo nginx -t
sudo systemctl status nginx

# Проверить файлы
ls -la /var/www/greenhouse-dashboard/

# Проверить логи
sudo tail -f /var/log/nginx/greenhouse_error.log
```

### Данные не сохраняются

```bash
# Проверить права
ls -la ~/greenhouse-data/

# Должно быть -rw-rw-rw-
chmod 666 ~/greenhouse-data/*.json

# Проверить путь в workflows
# Убедитесь что используется полный путь: /home/username/greenhouse-data/
```

---

## 📊 Мониторинг и Обслуживание

### Ежедневные задачи (автоматические)

- ✅ Workflow #5 автоматически очищает старые данные в 02:00
- ✅ Workflow #2 мониторит критические параметры каждые 20 секунд
- ✅ Telegram уведомления приходят автоматически

### Еженедельные задачи (вручную)

```bash
# Проверить размер файлов данных
du -sh ~/greenhouse-data/*

# Проверить логи Nginx
sudo du -sh /var/log/nginx/*

# Очистить старые логи (если нужно)
sudo logrotate -f /etc/logrotate.conf
```

### Ежемесячные задачи

```bash
# Обновить систему
sudo apt update && sudo apt upgrade -y

# Обновить n8n (если через npm)
npm update -g n8n

# Обновить workflows (если были изменения в репозитории)
cd ~/Balagan
git pull
# Затем переимпортировать workflows в UI
```

---

## 🎯 Следующие Шаги

1. ✅ Настроить DNS для домена (если еще не сделано)
2. ✅ Настроить SSL сертификат (Шаг 10)
3. ✅ Настроить автоматический backup данных
4. ✅ Добавить дополнительные Telegram команды (по желанию)
5. ✅ Интегрировать с Grafana (опционально)

---

## 📞 Поддержка

**Документация:**
- [README.md](../README.md) - полная документация
- [QUICK_START.md](QUICK_START.md) - быстрый старт
- [MQTT_DATA_STRUCTURE.md](MQTT_DATA_STRUCTURE.md) - структура данных

**Beget Support:**
- https://beget.com/ru/kb
- Тикеты в панели управления

**GitHub Issues:**
- https://github.com/Daydyaefim/Balagan/issues

---

## ✅ Checklist Развертывания

- [ ] n8n установлена через Beget Marketplace
- [ ] SSH доступ настроен
- [ ] Директория `/home/username/greenhouse-data/` создана
- [ ] Переменные окружения добавлены в `.env`
- [ ] n8n перезапущена
- [ ] 5 workflows импортированы
- [ ] MQTT credentials настроены
- [ ] Telegram credentials настроены
- [ ] Пути к файлам обновлены в workflows
- [ ] Все workflows активированы
- [ ] Nginx установлен и настроен
- [ ] Веб-дашборд развернут
- [ ] SSL сертификат установлен (опционально)
- [ ] Firewall настроен
- [ ] Telegram бот отвечает
- [ ] Веб-дашборд открывается
- [ ] MQTT данные поступают

---

**🎉 Поздравляем! Ваша система мониторинга теплицы развернута на Beget VPS!**
