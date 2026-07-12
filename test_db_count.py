import sqlite3
conn = sqlite3.connect(r'C:\Inventra\build\inventra.db')
c = conn.cursor()
c.execute('SELECT COUNT(*) FROM products')
print('DB Product Count:', c.fetchone()[0])
