import requests
import pandas as pd
import os
from datetime import datetime, timedelta
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv()

# Get API key from environment
API_KEY = os.getenv("POLYGON_API_KEY")
if not API_KEY:
    print("ERROR: POLYGON_API_KEY not found in .env file!")
    print("Please run setup_env.bat (Windows) or ./setup_env.sh (Linux/Mac) and add your API key to .env")
    exit(1)

symbol = "SPY"
end_date = datetime.today()
all_data = []

# Create data directory if it doesn't exist
os.makedirs("data", exist_ok=True)

# Go back day by day until no data is returned
max_days_without_data = 5  # Stop after this many consecutive days with no data
days_without_data = 0

print(f"Fetching {symbol} data using Polygon.io API...")
print(f"Starting from: {end_date.strftime('%Y-%m-%d')}")

while True:
    date_str = end_date.strftime("%Y-%m-%d")
    url = (
        f"https://api.polygon.io/v2/aggs/ticker/{symbol}/range/1/minute/"
        f"{date_str}/{date_str}?adjusted=true&sort=asc&limit=50000&apiKey={API_KEY}"
    )
    print(f"Fetching {date_str}...")
    
    try:
        resp = requests.get(url, timeout=30)
        if resp.status_code != 200:
            print(f"Failed to fetch {date_str}: {resp.text}")
            days_without_data += 1
        else:
            results = resp.json().get("results", [])
            if not results:
                print(f"No data for {date_str}")
                days_without_data += 1
            else:
                print(f"Got {len(results)} bars for {date_str}")
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
    except requests.exceptions.RequestException as e:
        print(f"Network error fetching {date_str}: {e}")
        days_without_data += 1
    except Exception as e:
        print(f"Error processing {date_str}: {e}")
        days_without_data += 1
    
    if days_without_data >= max_days_without_data:
        print("No data for several consecutive days. Stopping.")
        break
    end_date -= timedelta(days=1)

if not all_data:
    print("No data fetched!")
    exit(1)

df = pd.DataFrame(all_data)
df.sort_values("timestamp", inplace=True)
print(f"\nTotal rows fetched: {len(df)}")
print(f"Date range: {df['timestamp'].min()} to {df['timestamp'].max()}")
print("\nFirst 5 rows:")
print(df.head())
print("\nLast 5 rows:")
print(df.tail())

output_file = "data/SPY_1m.csv"
df.to_csv(output_file, index=False)
print(f"\nSaved {len(df)} rows to {output_file}")
print("Data fetch complete!")
