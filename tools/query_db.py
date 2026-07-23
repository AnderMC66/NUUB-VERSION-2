import sqlite3
import sys

db_path = r'C:\Users\Asus\.local\share\mimocode\mimocode.db'
conn = sqlite3.connect(db_path)
cursor = conn.cursor()

# List tables
cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
tables = cursor.fetchall()
print("=== TABLES ===")
for t in tables:
    print(t[0])

# Count recent sessions (last 30 days from 2026-07-22)
print("\n=== RECENT SESSIONS (last 30 days) ===")
cutoff_ms = 1749340800000  # 2026-06-08 approx
try:
    cursor.execute("SELECT id, time_created, title FROM session WHERE time_created > ?", (cutoff_ms,))
    sessions = cursor.fetchall()
    for s in sessions:
        print(f"  {s[0]} | ts={s[1]} | {s[2]}")
except Exception as e:
    print(f"  session table error: {e}")

# Count messages per session
print("\n=== MESSAGE COUNTS PER RECENT SESSION ===")
try:
    cursor.execute("""
        SELECT m.session_id, count(*) as cnt 
        FROM message m 
        WHERE m.time_created > ?
        GROUP BY m.session_id 
        ORDER BY cnt DESC
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  {r[0]}: {r[1]} messages")
except Exception as e:
    print(f"  message table error: {e}")

# Find repeated tool usage patterns
print("\n=== TOP REPEATED TOOL CALLS (last 30 days) ===")
try:
    cursor.execute("""
        SELECT json_extract(p.data, '$.tool') as tool,
               substr(json_extract(p.data, '$.state.input'), 1, 200) as input_preview,
               count(*) as n
        FROM message m
        JOIN part p ON p.message_id = m.id
        WHERE json_extract(m.data, '$.role') = 'assistant'
          AND json_extract(p.data, '$.type') = 'tool'
          AND m.time_created > ?
        GROUP BY tool, input_preview
        ORDER BY n DESC
        LIMIT 40
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        tool = r[0] or "null"
        inp = (r[1] or "null")[:120]
        print(f"  {r[2]:3d}x {tool}: {inp}")
except Exception as e:
    print(f"  tool query error: {e}")

# Find user messages with repeated keywords
print("\n=== USER MESSAGES WITH 'repeat'/'again'/'usual'/'like last time' KEYWORDS ===")
try:
    cursor.execute("""
        SELECT m.session_id, substr(json_extract(m.data, '$.content'), 1, 200)
        FROM message m
        WHERE json_extract(m.data, '$.role') = 'user'
          AND m.time_created > ?
          AND (
            json_extract(m.data, '$.content') LIKE '%again%'
            OR json_extract(m.data, '$.content') LIKE '%repeat%'
            OR json_extract(m.data, '$.content') LIKE '%usual%'
            OR json_extract(m.data, '$.content') LIKE '%like last time%'
            OR json_extract(m.data, '$.content') LIKE '%de nuevo%'
            OR json_extract(m.data, '$.content') LIKE '%otra vez%'
            OR json_extract(m.data, '$.content') LIKE '%como antes%'
            OR json_extract(m.data, '$.content') LIKE '%mismo que%'
          )
        LIMIT 20
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  [{r[0]}]: {(r[1] or '')[:180]}")
except Exception as e:
    print(f"  keyword query error: {e}")

# Repeated command sequences - find sessions that do similar things
print("\n=== REPEATED GIT/CMAKE/TEST COMMANDS ===")
try:
    cursor.execute("""
        SELECT json_extract(p.data, '$.tool') as tool,
               substr(json_extract(p.data, '$.state.input'), 1, 300) as input_preview,
               count(*) as n
        FROM message m
        JOIN part p ON p.message_id = m.id
        WHERE json_extract(m.data, '$.role') = 'assistant'
          AND json_extract(p.data, '$.type') = 'tool'
          AND json_extract(p.data, '$.tool') = 'bash'
          AND m.time_created > ?
        GROUP BY input_preview
        HAVING n >= 2
        ORDER BY n DESC
        LIMIT 30
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        inp = (r[0] or "null")[:200]
        print(f"  {r[1]:3d}x {inp}")
except Exception as e:
    print(f"  bash query error: {e}")

# Also check what write/edit calls are repeated
print("\n=== REPEATED FILE WRITES ===")
try:
    cursor.execute("""
        SELECT json_extract(p.data, '$.tool') as tool,
               substr(json_extract(p.data, '$.state.input'), 1, 300) as input_preview,
               count(*) as n
        FROM message m
        JOIN part p ON p.message_id = m.id
        WHERE json_extract(m.data, '$.role') = 'assistant'
          AND json_extract(p.data, '$.type') = 'tool'
          AND json_extract(p.data, '$.tool') IN ('write', 'edit')
          AND m.time_created > ?
        GROUP BY input_preview
        HAVING n >= 2
        ORDER BY n DESC
        LIMIT 30
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        tool = r[0] or "null"
        inp = (r[1] or "null")[:200]
        print(f"  {r[2]:3d}x {tool}: {inp}")
except Exception as e:
    print(f"  write query error: {e}")

conn.close()
