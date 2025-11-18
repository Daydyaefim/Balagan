# 🎯 КУДА КАКОЙ ПУТЬ УКАЗЫВАТЬ - ШПАРГАЛКА

## 📁 Ваш путь к файлам

**ВАЖНО!** Сначала определите ваш домашний путь:

```
/home/ВАШ_ЛОГИН/greenhouse-data/
```

**Как узнать свой логин:**
- В панели Beget посмотрите username
- Обычно это: `/home/john/` или `/home/user123/`

**Примеры:**
- Если логин `john` → путь: `/home/john/greenhouse-data/`
- Если логин `user123` → путь: `/home/user123/greenhouse-data/`

---

## 📝 Workflow #1: MQTT Data Pipeline

### Нода: "Читать БД"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_readings.json
```

### Нода: "Сохранить БД"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_readings.json
```

---

## 📝 Workflow #2: Critical Alerts Monitor

### Нода: "Читать Последние Данные"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_readings.json
```

### Нода: "Читать Состояния Оповещений"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/telegram_alert_states.json
```

### Нода: "Сохранить в Историю"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_alerts_history.json
```

### Нода: "Отправить в Telegram"
**Параметр:** `chatId`
**Вместо:** `={{ $env.TELEGRAM_CHAT_ID }}`
**Укажите:** `123456789` (ваш реальный Chat ID)

---

## 📝 Workflow #3: Telegram Callback Handler

### Нода: "Читать Состояния"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/telegram_alert_states.json
```

### Нода: "Сохранить Состояния"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/telegram_alert_states.json
```

---

## 📝 Workflow #4: Web Dashboard API

### Нода: "Читать БД"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_readings.json
```

---

## 📝 Workflow #5: Data Rotation & Cleanup

### Нода: "Читать Показания"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_readings.json
```

### Нода: "Читать Историю Оповещений"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_alerts_history.json
```

### Нода: "Сохранить Показания"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_readings.json
```

### Нода: "Сохранить Оповещения"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_alerts_history.json
```

### Нода: "Сохранить Лог"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/cleanup_log.json
```

### Нода: "Уведомить в Telegram"
**Параметр:** `chatId`
**Вместо:** `={{ $env.TELEGRAM_CHAT_ID }}`
**Укажите:** `123456789` (ваш реальный Chat ID)

---

## 📝 Workflow #6: Web Dashboard (Simple) ⭐

### Нода: "Читать Данные"
**Параметр:** `filePath`
**Путь:**
```
/home/ВАШ_ЛОГИН/greenhouse-data/ugagro_readings.json
```

---

## 🎯 Пример заполнения

### Если ваш логин: `john`

#### В ноде "Читать Данные" (Workflow #6):

1. Кликните на ноду "Читать Данные"
2. В правой панели найдите поле **File Path**
3. Вставьте:
```
/home/john/greenhouse-data/ugagro_readings.json
```
4. Нажмите **Save**

---

## 🔍 Как найти ноду с путем к файлу

### Визуально ноды выглядят так:

```
┌─────────────────────┐
│  📄 Читать Данные   │  ← Кликните на эту ноду
└─────────────────────┘
```

### После клика откроется панель справа:

```
┌──────────────────────────┐
│ Read/Write Files         │
│                          │
│ Operation: read          │
│                          │
│ File Path:               │
│ ┌──────────────────────┐ │
│ │ /data/ugagro_...     │ │ ← Измените этот путь
│ └──────────────────────┘ │
│                          │
│          [Save]          │
└──────────────────────────┘
```

---

## ✅ Checklist изменений путей

- [ ] Workflow #1 - 2 пути изменены
- [ ] Workflow #2 - 3 пути + 1 Chat ID изменены
- [ ] Workflow #3 - 2 пути изменены
- [ ] Workflow #4 - 1 путь изменен
- [ ] Workflow #5 - 4 пути + 1 Chat ID изменены
- [ ] Workflow #6 - 1 путь изменен

**Всего: 13 путей + 2 Chat ID**

---

## 💡 Совет

Скопируйте ваш путь в блокнот:
```
/home/ВАШ_ЛОГИН/greenhouse-data/
```

Затем просто добавляйте имя файла:
- `ugagro_readings.json`
- `telegram_alert_states.json`
- `ugagro_alerts_history.json`
- `cleanup_log.json`

---

## ❓ Проблемы?

### Ошибка "File not found"
- Проверьте логин правильно ли указан
- Создайте файлы через Beget File Manager

### Ошибка "Permission denied"
- В Beget File Manager кликните на файл правой кнопкой → Permissions → 666

---

**Готово! Теперь у вас есть точная карта где что менять!** 🎉
