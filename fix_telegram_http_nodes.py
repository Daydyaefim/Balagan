#!/usr/bin/env python3
"""
Заменяем HTTP Request ноды на Code ноды для надежной отправки данных в Telegram
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/telegram-callback-handler.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Заменяем ноды
for i, node in enumerate(workflow['nodes']):
    if node['id'] == 'answer-callback':
        # Заменяем на Code ноду
        workflow['nodes'][i] = {
            "parameters": {
                "jsCode": """// Отвечаем на callback query
const data = $input.first().json;

const response = await $http.request({
  method: 'POST',
  url: 'https://api.telegram.org/bot<ВАШ_ТОКЕН>/answerCallbackQuery',
  headers: {
    'Content-Type': 'application/json'
  },
  body: {
    callback_query_id: data.callback_query_id,
    text: 'Обработано ✓'
  }
});

return $input.all();"""
            },
            "id": "answer-callback",
            "name": "Ответить на Callback",
            "type": "n8n-nodes-base.code",
            "typeVersion": 2,
            "position": node['position']
        }
        print("✓ Заменена нода 'Ответить на Callback' на Code")

    elif node['id'] == 'edit-message':
        # Заменяем на Code ноду
        workflow['nodes'][i] = {
            "parameters": {
                "jsCode": """// Редактируем сообщение в Telegram
const data = $input.first().json;

const response = await $http.request({
  method: 'POST',
  url: 'https://api.telegram.org/bot<ВАШ_ТОКЕН>/editMessageText',
  headers: {
    'Content-Type': 'application/json'
  },
  body: {
    chat_id: data.chat_id,
    message_id: data.message_id,
    text: data.message,
    parse_mode: 'Markdown'
  }
});

return [{
  json: {
    ...data,
    telegram_response: response
  }
}];"""
            },
            "id": "edit-message",
            "name": "Изменить Сообщение",
            "type": "n8n-nodes-base.code",
            "typeVersion": 2,
            "position": node['position']
        }
        print("✓ Заменена нода 'Изменить Сообщение' на Code")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/telegram-callback-handler.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow исправлен!")
print("\nИзменения:")
print("  • HTTP Request ноды заменены на Code ноды")
print("  • Используется встроенный $http.request()")
print("  • Body передается как JavaScript объект")
print("  • Telegram API получит правильные параметры")
print("\nТеперь данные будут передаваться надежно!")
