import sqlite3
import time

def test():
    print("Testing DB lock...")
    try:
        conn = sqlite3.connect("build/Inventra.app/Contents/MacOS/inventra.db")
        conn.execute("INSERT INTO products (sku, name) VALUES ('TEST-123', 'Test')")
        conn.commit()
        conn.close()
        print("Success!")
    except Exception as e:
        print("Error:", e)
test()
