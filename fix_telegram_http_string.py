#!/usr/bin/env python3
"""
Исправляем HTTP Request ноды - используем specifyBody: "string" вместо "json"
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/telegram-callback-handler.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Заменяем Code ноды обратно на HTTP Request с правильной настройкой
for i, node in enumerate(workflow['nodes']):
    if node['id'] == 'answer-callback':
        # HTTP Request с string body
        workflow['nodes'][i] = {
            "parameters": {
                "method": "POST",
                "url": "=https://api.telegram.org/bot<ВАШ_ТОКЕН>/answerCallbackQuery",
                "authentication": "none",
                "sendBody": True,
                "specifyBody": "string",
                "bodyContentType": "json",
                "body": "={{ JSON.stringify({ callback_query_id: $json.callback_query_id, text: 'Обработано ✓' }) }}",
                "options": {}
            },
            "id": "answer-callback",
            "name": "Ответить на Callback",
            "type": "n8n-nodes-base.httpRequest",
            "typeVersion": 4.2,
            "position": node['position']
        }
        print("✓ Исправлена нода 'Ответить на Callback' (string body)")

    elif node['id'] == 'edit-message':
        # HTTP Request с string body
        workflow['nodes'][i] = {
            "parameters": {
                "method": "POST",
                "url": "=https://api.telegram.org/bot<ВАШ_ТОКЕН>/editMessageText",
                "authentication": "none",
                "sendBody": True,
                "specifyBody": "string",
                "bodyContentType": "json",
                "body": "={{ JSON.stringify({ chat_id: $json.chat_id, message_id: $json.message_id, text: $json.message, parse_mode: 'Markdown' }) }}",
                "options": {}
            },
            "id": "edit-message",
            "name": "Изменить Сообщение",
            "type": "n8n-nodes-base.httpRequest",
            "typeVersion": 4.2,
            "position": node['position']
        }
        print("✓ Исправлена нода 'Изменить Сообщение' (string body)")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/telegram-callback-handler.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow исправлен!")
print("\nИзменения:")
print("  • Используем HTTP Request ноды")
print("  • specifyBody: 'string' вместо 'json'")
print("  • bodyContentType: 'json' для правильного Content-Type")
print("  • body с JSON.stringify() для сериализации")
print("\nТеперь данные будут передаваться корректно!")
