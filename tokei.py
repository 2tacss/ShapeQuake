import json
import sqlite3
import subprocess
import sys
from datetime import datetime
from pathlib import Path

DB_PATH = Path("tokei_history.db")

def init_db():
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS tokei_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            language TEXT NOT NULL,
            files INTEGER NOT NULL,
            code INTEGER NOT NULL,
            comments INTEGER NOT NULL,
            blanks INTEGER NOT NULL,
            lines INTEGER NOT NULL
        )
    """)
    conn.commit()
    conn.close()

def collect_tokei_data():
    try:
        result = subprocess.run(
            ["tokei", "-o", "json"],
            capture_output=True,
            text=True,
            check=True
        )
        return json.loads(result.stdout)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"[ ! ] Failed to execute tokei: {e}")
        return None

def log_to_sqlite(data):
    if not data:
        print("[ ! ] Invalid tokei data.")
        return

    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    insert_entries = []
    
    for key, val in data.items():
        if key == "Total":
            continue
            
        code = val.get("code", 0)
        comments = val.get("comments", 0)
        blanks = val.get("blanks", 0)
        lines = code + comments + blanks
        
        files = 0
        if "Total" in data and "children" in data["Total"]:
            children_data = data["Total"]["children"]
            child_key = key.replace(" ", "") if key == "C Header" and "CHeader" in children_data else key
            if child_key in children_data:
                files = len(children_data[child_key])

        insert_entries.append((
            current_time, key, files, code, comments, blanks, lines
        ))
    
    if not insert_entries:
        conn.close()
        return

    cursor.executemany("""
        INSERT INTO tokei_logs (timestamp, language, files, code, comments, blanks, lines)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    """, insert_entries)
    conn.commit()
    print(f"[ * ] Successfully logged {len(insert_entries)} languages to {DB_PATH} at {current_time}.")
    conn.close()

def view_symmetry():
    if not DB_PATH.exists():
        print("[ ! ] No database found. Run a log first.")
        return

    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    
    cursor.execute("SELECT timestamp, language, files, code, comments, blanks, lines FROM tokei_logs ORDER BY timestamp ASC")
    rows = cursor.fetchall()
    conn.close()

    history = {}
    for timestamp, lang, files, code, comments, blanks, lines in rows:
        if timestamp not in history:
            history[timestamp] = {"C": None, "C Header": None}
        if lang in ["C", "C Header"]:
            history[timestamp][lang] = {
                "files": files, "code": code, "comments": comments, "blanks": blanks, "lines": lines
            }

    print(f"{'Date':<20} | {'C:F':<3} {'H:F':<3} | {'C:Code':<6} {'H:Code':<6} | {'C:Com':<5} {'H:Com':<5} | {'C:Blk':<5} {'H:Blk':<5} | {'C:Line':<6} {'H:Line':<6} | {'Ratio':<5}")
    print("-" * 115)

    for ts, langs in history.items():
        c = langs["C"]
        h = langs["C Header"]
        
        if c and h:
            ratio = round(c["code"] / h["code"], 2) if h["code"] > 0 else 0.0
            
            print(f"{ts:<20} | "
                  f"{c['files']:<3} {h['files']:<3} | "
                  f"{c['code']:<6} {h['code']:<6} | "
                  f"{c['comments']:<5} {h['comments']:<5} | "
                  f"{c['blanks']:<5} {h['blanks']:<5} | "
                  f"{c['lines']:<6} {h['lines']:<6} | "
                  f"{ratio:<5}")

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--view":
        view_symmetry()
    else:
        init_db()
        tokei_data = collect_tokei_data()
        if tokei_data:
            log_to_sqlite(tokei_data)
