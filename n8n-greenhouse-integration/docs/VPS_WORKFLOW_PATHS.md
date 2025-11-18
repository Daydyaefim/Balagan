# 🔧 Изменение Путей в Workflows для VPS

## Автоматическая замена путей

После импорта workflows в n8n на Beget VPS, необходимо обновить пути к файлам данных.

---

## 📝 Что нужно изменить

### Было (Docker):
```
/data/ugagro_readings.json
/data/telegram_alert_states.json
/data/ugagro_alerts_history.json
/data/cleanup_log.json
```

### Стало (Beget VPS):
```
/home/USERNAME/greenhouse-data/ugagro_readings.json
/home/USERNAME/greenhouse-data/telegram_alert_states.json
/home/USERNAME/greenhouse-data/ugagro_alerts_history.json
/home/USERNAME/greenhouse-data/cleanup_log.json
```

**ВАЖНО:** Замените `USERNAME` на ваш фактический логин в системе!

---

## 🔄 Workflow #1: MQTT Data Pipeline

### Node: "Читать БД"
**Найти:**
```json
{
  "operation": "read",
  "filePath": "/data/ugagro_readings.json"
}
```

**Заменить на:**
```json
{
  "operation": "read",
  "filePath": "/home/USERNAME/greenhouse-data/ugagro_readings.json"
}
```

### Node: "Сохранить БД"
**Найти:**
```json
{
  "operation": "write",
  "filePath": "/data/ugagro_readings.json"
}
```

**Заменить на:**
```json
{
  "operation": "write",
  "filePath": "/home/USERNAME/greenhouse-data/ugagro_readings.json"
}
```

---

## 🔄 Workflow #2: Critical Alerts Monitor

### Node: "Читать Последние Данные"
**Найти:**
```json
{
  "operation": "read",
  "filePath": "/data/ugagro_readings.json"
}
```

**Заменить на:**
```json
{
  "operation": "read",
  "filePath": "/home/USERNAME/greenhouse-data/ugagro_readings.json"
}
```

### Node: "Читать Состояния Оповещений"
**Найти:**
```json
{
  "operation": "read",
  "filePath": "/data/telegram_alert_states.json"
}
```

**Заменить на:**
```json
{
  "operation": "read",
  "filePath": "/home/USERNAME/greenhouse-data/telegram_alert_states.json"
}
```

### Node: "Сохранить в Историю"
**Найти:**
```json
{
  "operation": "write",
  "filePath": "/data/ugagro_alerts_history.json"
}
```

**Заменить на:**
```json
{
  "operation": "write",
  "filePath": "/home/USERNAME/greenhouse-data/ugagro_alerts_history.json"
}
```

---

## 🔄 Workflow #3: Telegram Callback Handler

### Node: "Читать Состояния"
**Найти:**
```json
{
  "operation": "read",
  "filePath": "/data/telegram_alert_states.json"
}
```

**Заменить на:**
```json
{
  "operation": "read",
  "filePath": "/home/USERNAME/greenhouse-data/telegram_alert_states.json"
}
```

### Node: "Сохранить Состояния"
**Найти:**
```json
{
  "operation": "write",
  "filePath": "/data/telegram_alert_states.json"
}
```

**Заменить на:**
```json
{
  "operation": "write",
  "filePath": "/home/USERNAME/greenhouse-data/telegram_alert_states.json"
}
```

---

## 🔄 Workflow #4: Web Dashboard API

### Node: "Читать БД"
**Найти:**
```json
{
  "operation": "read",
  "filePath": "/data/ugagro_readings.json"
}
```

**Заменить на:**
```json
{
  "operation": "read",
  "filePath": "/home/USERNAME/greenhouse-data/ugagro_readings.json"
}
```

---

## 🔄 Workflow #5: Data Rotation & Cleanup

### Node: "Читать Показания"
**Найти:**
```json
{
  "operation": "read",
  "filePath": "/data/ugagro_readings.json"
}
```

**Заменить на:**
```json
{
  "operation": "read",
  "filePath": "/home/USERNAME/greenhouse-data/ugagro_readings.json"
}
```

### Node: "Читать Историю Оповещений"
**Найти:**
```json
{
  "operation": "read",
  "filePath": "/data/ugagro_alerts_history.json"
}
```

**Заменить на:**
```json
{
  "operation": "read",
  "filePath": "/home/USERNAME/greenhouse-data/ugagro_alerts_history.json"
}
```

### Node: "Сохранить Показания"
**Найти:**
```json
{
  "operation": "write",
  "filePath": "/data/ugagro_readings.json"
}
```

**Заменить на:**
```json
{
  "operation": "write",
  "filePath": "/home/USERNAME/greenhouse-data/ugagro_readings.json"
}
```

### Node: "Сохранить Оповещения"
**Найти:**
```json
{
  "operation": "write",
  "filePath": "/data/ugagro_alerts_history.json"
}
```

**Заменить на:**
```json
{
  "operation": "write",
  "filePath": "/home/USERNAME/greenhouse-data/ugagro_alerts_history.json"
}
```

### Node: "Сохранить Лог"
**Найти:**
```json
{
  "operation": "write",
  "filePath": "/data/cleanup_log.json"
}
```

**Заменить на:**
```json
{
  "operation": "write",
  "filePath": "/home/USERNAME/greenhouse-data/cleanup_log.json"
}
```

---

## ✅ Checklist изменений

- [ ] Workflow #1 - MQTT Data Pipeline (2 изменения)
- [ ] Workflow #2 - Critical Alerts Monitor (3 изменения)
- [ ] Workflow #3 - Telegram Callback Handler (2 изменения)
- [ ] Workflow #4 - Web Dashboard API (1 изменение)
- [ ] Workflow #5 - Data Rotation & Cleanup (5 изменений)

**Всего: 13 изменений**

---

## 🎯 Как применить изменения

### Способ 1: Через UI (рекомендуется)

1. Откройте workflow в n8n
2. Найдите соответствующий Node (например, "Читать БД")
3. Кликните на Node
4. Найдите параметр `filePath`
5. Замените `/data/` на `/home/USERNAME/greenhouse-data/`
6. Нажмите **Save**
7. Повторите для всех указанных Nodes

### Способ 2: Через экспорт/импорт

1. Экспортируйте workflow из n8n (JSON)
2. Откройте JSON в текстовом редакторе
3. Используйте Find & Replace:
   - Find: `/data/`
   - Replace: `/home/USERNAME/greenhouse-data/`
4. Сохраните JSON
5. Удалите старый workflow в n8n
6. Импортируйте обновленный JSON

### Способ 3: Через SSH (продвинутый)

```bash
# Подключитесь к VPS
ssh username@your-vps-ip

# Найдите директорию n8n workflows
cd ~/.n8n/workflows

# Создайте backup
cp -r . ../workflows_backup

# Замените пути (опасно! сделайте backup!)
sed -i 's|/data/|/home/USERNAME/greenhouse-data/|g' *.json

# Перезапустите n8n
sudo systemctl restart n8n
```

**⚠️ Внимание:** Способ 3 рискованный! Используйте только если уверены.

---

## 🔍 Проверка изменений

После внесения изменений:

1. Активируйте workflow
2. Выполните тестовый запуск (Execute Workflow)
3. Проверьте, что данные пишутся в правильную директорию:

```bash
# На VPS
ls -la ~/greenhouse-data/
cat ~/greenhouse-data/ugagro_readings.json | jq . | tail -10
```

4. Проверьте логи n8n на наличие ошибок:

```bash
sudo journalctl -u n8n -f
```

---

## ❗ Частые ошибки

### Ошибка: "Permission denied"

**Причина:** Неправильные права на файлы

**Решение:**
```bash
chmod 666 ~/greenhouse-data/*.json
```

### Ошибка: "File not found"

**Причина:** Неправильный путь или файл не создан

**Решение:**
```bash
# Создайте файл
touch ~/greenhouse-data/ugagro_readings.json

# Проверьте путь
echo $HOME
# Используйте полный путь: /home/username/greenhouse-data/
```

### Ошибка: "ENOSPC: no space left on device"

**Причина:** Закончилось место на диске

**Решение:**
```bash
# Проверьте место
df -h

# Очистите старые данные (ОСТОРОЖНО!)
# Workflow #5 должен делать это автоматически
```

---

**✅ После внесения всех изменений ваши workflows будут работать корректно на Beget VPS!**
