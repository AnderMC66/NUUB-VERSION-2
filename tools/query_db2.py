import sqlite3

db_path = r'C:\Users\Asus\.local\share\mimocode\mimocode.db'
conn = sqlite3.connect(db_path)
cursor = conn.cursor()

cutoff_ms = 1749340800000  # 2026-06-08 approx (30 days before 2026-07-22)

# Repeated bash command sequences
print("=== REPEATED BASH COMMANDS (>= 2 occurrences) ===")
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
        GROUP BY json_extract(p.data, '$.state.input')
        HAVING count(*) >= 2
        ORDER BY n DESC
        LIMIT 30
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        inp = (r[0] or "null")[:250]
        print(f"  {r[1]:3d}x: {inp}")
except Exception as e:
    print(f"  Error: {e}")

# CMake build command patterns
print("\n=== CMAKE BUILD COMMANDS ===")
try:
    cursor.execute("""
        SELECT substr(json_extract(p.data, '$.state.input'), 1, 400) as inp,
               count(*) as n
        FROM message m
        JOIN part p ON p.message_id = m.id
        WHERE json_extract(m.data, '$.role') = 'assistant'
          AND json_extract(p.data, '$.type') = 'tool'
          AND json_extract(p.data, '$.tool') = 'bash'
          AND json_extract(p.data, '$.state.input') LIKE '%cmake%'
          AND m.time_created > ?
        GROUP BY inp
        HAVING count(*) >= 2
        ORDER BY n DESC
        LIMIT 20
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  {r[1]:3d}x: {(r[0] or '')[:350]}")
except Exception as e:
    print(f"  Error: {e}")

# Git commands
print("\n=== GIT COMMANDS ===")
try:
    cursor.execute("""
        SELECT substr(json_extract(p.data, '$.state.input'), 1, 400) as inp,
               count(*) as n
        FROM message m
        JOIN part p ON p.message_id = m.id
        WHERE json_extract(m.data, '$.role') = 'assistant'
          AND json_extract(p.data, '$.type') = 'tool'
          AND json_extract(p.data, '$.tool') = 'bash'
          AND json_extract(p.data, '$.state.input') LIKE '%git%'
          AND m.time_created > ?
        GROUP BY inp
        HAVING count(*) >= 2
        ORDER BY n DESC
        LIMIT 20
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  {r[1]:3d}x: {(r[0] or '')[:350]}")
except Exception as e:
    print(f"  Error: {e}")

# Find unique user session titles/topics
print("\n=== NUUB-VERSION-2 RELATED SESSIONS (last 30 days) ===")
try:
    cursor.execute("""
        SELECT s.id, s.title, s.time_created
        FROM session s
        WHERE s.time_created > ?
          AND (
            s.title LIKE '%nuub%'
            OR s.title LIKE '%C++%'
            OR s.title LIKE '%DDD%'
            OR s.title LIKE '%RAT%'
            OR s.title LIKE '%model-nubb%'
            OR s.title LIKE '%encript%'
            OR s.title LIKE '%compil%'
            OR s.title LIKE '%build%'
            OR s.title LIKE '%github%'
            OR s.title LIKE '%release%'
            OR s.title LIKE '%test%'
            OR s.title LIKE '%clean architecture%'
            OR s.title LIKE '%MCP%'
          )
        ORDER BY s.time_created DESC
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  [{r[0]}] ts={r[1]}: {r[2]}")
except Exception as e:
    print(f"  Error: {e}")

# Also search for user messages about NUUB specifically
print("\n=== USER MESSAGES MENTIONING NUUB/C++ ===")
try:
    cursor.execute("""
        SELECT m.session_id, substr(json_extract(m.data, '$.content'), 1, 300)
        FROM message m
        WHERE json_extract(m.data, '$.role') = 'user'
          AND m.time_created > ?
          AND (
            json_extract(m.data, '$.content') LIKE '%nuub%'
            OR json_extract(m.data, '$.content') LIKE '%C++%'
            OR json_extract(m.data, '$.content') LIKE '%cmake%'
            OR json_extract(m.data, '$.content') LIKE '%compil%'
          )
        LIMIT 20
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  [{r[0]}]: {(r[1] or '')[:250]}")
except Exception as e:
    print(f"  Error: {e}")

# Check sessions focused on this specific project
print("\n=== NUUB SESSIONS WITH BUILD/TEST ACTIVITY ===")
try:
    cursor.execute("""
        SELECT m.session_id, count(*) as tool_calls
        FROM message m
        JOIN part p ON p.message_id = m.id
        WHERE json_extract(m.data, '$.role') = 'assistant'
          AND json_extract(p.data, '$.type') = 'tool'
          AND json_extract(p.data, '$.tool') = 'bash'
          AND json_extract(p.data, '$.state.input') LIKE '%cmake%'
          AND m.time_created > ?
        GROUP BY m.session_id
        ORDER BY tool_calls DESC
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  [{r[0]}]: {r[1]} cmake calls")
except Exception as e:
    print(f"  Error: {e}")

# Read/edit patterns for CMakeLists.txt
print("\n=== CMAKELists.txt EDITS ===")
try:
    cursor.execute("""
        SELECT m.session_id, json_extract(p.data, '$.tool') as tool,
               substr(json_extract(p.data, '$.state.input'), 1, 200) as inp,
               count(*) as n
        FROM message m
        JOIN part p ON p.message_id = m.id
        WHERE json_extract(m.data, '$.role') = 'assistant'
          AND json_extract(p.data, '$.type') = 'tool'
          AND json_extract(p.data, '$.tool') IN ('read', 'edit', 'write')
          AND json_extract(p.data, '$.state.input') LIKE '%CMakeLists%'
          AND m.time_created > ?
        GROUP BY m.session_id, tool
        ORDER BY n DESC
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  [{r[0]}] {r[1]}: {r[3]}x")
except Exception as e:
    print(f"  Error: {e}")

# Repeated file write patterns (C++ files)
print("\n=== C++ FILE WRITE PATTERNS ===")
try:
    cursor.execute("""
        SELECT json_extract(p.data, '$.tool') as tool,
               substr(json_extract(p.data, '$.state.input'), 1, 200) as inp,
               count(*) as n
        FROM message m
        JOIN part p ON p.message_id = m.id
        WHERE json_extract(m.data, '$.role') = 'assistant'
          AND json_extract(p.data, '$.type') = 'tool'
          AND json_extract(p.data, '$.tool') IN ('write', 'edit')
          AND (json_extract(p.data, '$.state.input') LIKE '%.cpp%'
               OR json_extract(p.data, '$.state.input') LIKE '%.hpp%')
          AND m.time_created > ?
        GROUP BY json_extract(p.data, '$.state.input')
        HAVING count(*) >= 2
        ORDER BY n DESC
        LIMIT 20
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  {r[2]:3d}x {r[0]}: {(r[1] or '')[:180]}")
except Exception as e:
    print(f"  Error: {e}")

# Read the most recent checkpoint for the current project
print("\n=== MOST RECENT NUUB-VERSION-2 SESSIONS ===")
try:
    cursor.execute("""
        SELECT s.id, s.title, s.time_created, s.directory
        FROM session s
        WHERE s.time_created > ?
          AND s.directory LIKE '%NUUB%'
        ORDER BY s.time_created DESC
        LIMIT 10
    """, (cutoff_ms,))
    for r in cursor.fetchall():
        print(f"  [{r[0]}] ts={r[1]} dir={r[3]}")
except Exception as e:
    print(f"  Error: {e}")

conn.close()
