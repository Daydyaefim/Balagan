#!/usr/bin/env python3
"""
Используем Code ноды с нативным fetch вместо HTTP Request
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/telegram-callback-handler.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Заменяем HTTP Request ноды на Code ноды с fetch
for i, node in enumerate(workflow['nodes']):
    if node['id'] == 'answer-callback':
        workflow['nodes'][i] = {
            "parameters": {
                "jsCode": """// Отвечаем на callback query
const data = $input.first().json;

const response = await fetch('https://api.telegram.org/bot<ВАШ_ТОКЕН>/answerCallbackQuery', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    callback_query_id: data.callback_query_id,
    text: 'Обработано ✓'
  })
});

const result = await response.json();
console.log('Answer callback result:', result);

return $input.all();"""
            },
            "id": "answer-callback",
            "name": "Ответить на Callback",
            "type": "n8n-nodes-base.code",
            "typeVersion": 2,
            "position": node['position']
        }
        print("✓ Заменена нода 'Ответить на Callback' (fetch)")

    elif node['id'] == 'edit-message':
        workflow['nodes'][i] = {
            "parameters": {
                "jsCode": """// Редактируем сообщение в Telegram
const data = $input.first().json;

const response = await fetch('https://api.telegram.org/bot<ВАШ_ТОКЕН>/editMessageText', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    chat_id: data.chat_id,
    message_id: data.message_id,
    text: data.message,
    parse_mode: 'Markdown'
  })
});

const result = await response.json();
console.log('Edit message result:', result);

if (!result.ok) {
  throw new Error(`Telegram API error: ${result.description}`);
}

return [{
  json: {
    ...data,
    telegram_response: result
  }
}];"""
            },
            "id": "edit-message",
            "name": "Изменить Сообщение",
            "type": "n8n-nodes-base.code",
            "typeVersion": 2,
            "position": node['position']
        }
        print("✓ Заменена нода 'Изменить Сообщение' (fetch)")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/telegram-callback-handler.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow исправлен!")
print("\nИзменения:")
print("  • Используем Code ноды с нативным fetch()")
print("  • fetch() доступен в n8n Code ноде")
print("  • Явная сериализация JSON.stringify()")
print("  • Правильный Content-Type заголовок")
print("\nТеперь запросы должны работать корректно!")
