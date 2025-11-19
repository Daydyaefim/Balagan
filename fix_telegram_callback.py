#!/usr/bin/env python3
"""
Fix Telegram callback handler - remove JSON.stringify from jsonBody
"""

import json

# Читаем workflow
with open('n8n-greenhouse-integration/workflows/telegram-callback-handler.json', 'r', encoding='utf-8') as f:
    workflow = json.load(f)

# Находим и исправляем ноды HTTP Request
for node in workflow['nodes']:
    if node['id'] == 'answer-callback':
        # Убираем JSON.stringify из jsonBody
        node['parameters']['jsonBody'] = "={{ { callback_query_id: $json.callback_query_id, text: 'Обработано ✓' } }}"
        print("✓ Исправлена нода 'Ответить на Callback'")

    elif node['id'] == 'edit-message':
        # Убираем JSON.stringify из jsonBody
        node['parameters']['jsonBody'] = "={{ { chat_id: $json.chat_id, message_id: $json.message_id, text: $json.message, parse_mode: 'Markdown' } }}"
        print("✓ Исправлена нода 'Изменить Сообщение'")

# Записываем обратно
with open('n8n-greenhouse-integration/workflows/telegram-callback-handler.json', 'w', encoding='utf-8') as f:
    json.dump(workflow, f, ensure_ascii=False, indent=2)

print("\n✅ Workflow исправлен!")
print("\nПроблема:")
print("  • JSON.stringify() использовался с specifyBody: 'json'")
print("  • n8n ожидает объект, а не JSON строку")
print("  • Это приводило к пустому message text")
print("\nРешение:")
print("  • Убран JSON.stringify() из jsonBody")
print("  • Теперь передаётся объект напрямую")
print("  • Telegram API получит правильный text")
