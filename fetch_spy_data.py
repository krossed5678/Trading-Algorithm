import requests
import pandas as pd
from datetime import datetime, timedelta

API_KEY = "kCpppxilaD50IkvDvWC5Dkj9kw7hhYpX"  # <-- Your Polygon.io API key
symbol = "SPY"
end_date = datetime.today()
all_data = []

# Go back day by day until no data is returned
max_days_without_data = 5  # Stop after this many consecutive days with no data
days_without_data = 0

while True:
    date_str = end_date.strftime("%Y-%m-%d")
    url = (
        f"https://api.polygon.io/v2/aggs/ticker/{symbol}/range/1/minute/"
        f"{date_str}/{date_str}?adjusted=true&sort=asc&limit=50000&apiKey={API_KEY}"
    )
    print(f"Fetching {date_str}...")
    resp = requests.get(url)
    if resp.status_code != 200:
        print(f"Failed to fetch {date_str}: {resp.text}")
        days_without_data += 1
    else:
        results = resp.json().get("results", [])
        if not results:
            print(f"No data for {date_str}")
            days_without_data += 1
        else:
            for bar in results:
                all_data.append({
                    "timestamp": datetime.utcfromtimestamp(bar["t"] / 1000).strftime("%Y-%m-%d %H:%M:%S"),
                    "open": bar["o"],
                    "high": bar["h"],
                    "low": bar["l"],
                    "close": bar["c"],
                    "volume": bar["v"]
                })
            days_without_data = 0  # Reset if we got data
    if days_without_data >= max_days_without_data:
        print("No data for several consecutive days. Stopping.")
        break
    end_date -= timedelta(days=1)

if not all_data:
    print("No data fetched!")
    exit(1)

df = pd.DataFrame(all_data)
df.sort_values("timestamp", inplace=True)
print("Rows in DataFrame:", len(df))
print(df.head())
print(df.tail())

df.to_csv("data/SPY_1m.csv", index=False)
print("Saved to data/SPY_1m.csv")
