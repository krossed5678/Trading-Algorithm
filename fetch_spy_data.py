import os
import requests
import csv
from datetime import datetime, timedelta
import time
from dotenv import load_dotenv

load_dotenv()

API_KEY = os.getenv("POLYGON_API_KEY")
if not API_KEY:
    raise RuntimeError("POLYGON_API_KEY not set in environment. Please set it in your .env file or environment variables.")

symbol = "SPY"
interval = "minute"
output_file = "data/SPY_1m.csv"

# Polygon.io v2 aggregates endpoint
BASE_URL = "https://api.polygon.io/v2/aggs/ticker/{}/range/1/minute/{}/{}?adjusted=true&sort=desc&limit=50000&apiKey={}"

# Start from today and go backwards
end_date = datetime.utcnow()
chunk_days = 7  # Use small chunks for short API windows

def fetch_chunk(symbol, from_date, to_date, api_key):
    url = BASE_URL.format(symbol, from_date.strftime("%Y-%m-%d"), to_date.strftime("%Y-%m-%d"), api_key)
    print(f"[INFO] Fetching {symbol} 1m data: {from_date.date()} to {to_date.date()} ...")
    r = requests.get(url)
    if r.status_code != 200:
        print(f"[ERROR] Failed to fetch data: {r.status_code} {r.text}")
        return []
    data = r.json()
    if "results" not in data:
        print(f"[WARNING] No results for {from_date.date()} to {to_date.date()}")
        return []
    return data["results"]

def write_csv_header(filename):
    with open(filename, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "open", "high", "low", "close", "volume"])

def append_csv_rows(filename, rows):
    with open(filename, "a", newline="") as f:
        writer = csv.writer(f)
        for row in rows:
            writer.writerow(row)

def main():
    os.makedirs("data", exist_ok=True)
    all_rows = []
    current_end = end_date
    total_rows = 0
    while True:
        current_start = current_end - timedelta(days=chunk_days)
        chunk = fetch_chunk(symbol, current_start, current_end, API_KEY)
        if not chunk:
            print("[INFO] No more data returned. Stopping.")
            break
        rows = []
        for bar in chunk:
            ts = datetime.utcfromtimestamp(bar["t"] / 1000).strftime("%Y-%m-%d %H:%M:%S")
            rows.append([
                ts,
                bar["o"],
                bar["h"],
                bar["l"],
                bar["c"],
                bar["v"]
            ])
        all_rows = rows + all_rows  # Prepend so oldest is first
        total_rows += len(rows)
        print(f"[INFO] Fetched {len(rows)} rows ({total_rows} total)")
        current_end = current_start - timedelta(days=1)
        time.sleep(1)  # Avoid rate limits
    print(f"[DONE] Writing {len(all_rows)} rows to {output_file}")
    write_csv_header(output_file)
    append_csv_rows(output_file, all_rows)
    print(f"[DONE] All data written to {output_file}")

if __name__ == "__main__":
    main()
