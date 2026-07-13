import sqlite3
conn = sqlite3.connect(r'C:\Inventra\build\inventra.db')
cur = conn.cursor()
cur.execute("SELECT name FROM sqlite_master WHERE type='table';")
print('Tables:', [t[0] for t in cur.fetchall()])
try:
    cur.execute("SELECT username FROM users")
    print('Users:', [t[0] for t in cur.fetchall()])
except Exception as e:
    print(e)
