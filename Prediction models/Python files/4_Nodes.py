#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import gspread
from oauth2client.service_account import ServiceAccountCredentials
import time
from datetime import datetime, timezone
import pytz
from math import radians, sin, cos, sqrt, atan2

# Define weather station coordinates
stations_info = {
    'WS1': {'latitude': 7.0193689, 'longitude': 79.9001577},
    'WS2': {'latitude': 7.0193110, 'longitude': 79.9002777},
    'WS3': {'latitude': 7.0197988, 'longitude': 79.9002482},
    'WS4': {'latitude': 7.0198337, 'longitude': 79.9001282}
}

# Power parameter for IDW
p = 2

# Google Sheets credentials and access
scope = ["https://spreadsheets.google.com/feeds", "https://www.googleapis.com/auth/drive"]
creds = ServiceAccountCredentials.from_json_keyfile_name("your_google_credentials1.json", scope)
client = gspread.authorize(creds)

# Google Sheets references
sheets_info = {
    'WS1': client.open("WS1").sheet1,
    'WS2': client.open("WS2").sheet1,
    'WS3': client.open("WS3").sheet1,
    'WS4': client.open("WS4").sheet1,
    'pre4': client.open("pre4").sheet1
}

# Set your local timezone
local_tz = pytz.timezone('Asia/Colombo')

# Define expected headers
WS_EXPECTED_HEADERS = ['Date', 'Time', 'Temperature', 'Humidity', 'Air Pressure', 'Air Quality', 'Rain Status']
PD_EXPECTED_HEADERS = ['Date', 'Time', 'Latitude', 'Longitude', 'Temperature', 'Humidity', 'Air Pressure', 'Air Quality', 'Rain Status']

def get_user_location():
    while True:
        try:
            lat = float(input("Enter the latitude of the target location: "))
            lon = float(input("Enter the longitude of the target location: "))
            if -90 <= lat <= 90 and -180 <= lon <= 180:
                return {'latitude': lat, 'longitude': lon}
            else:
                print("Invalid coordinates. Please enter valid latitude and longitude.")
        except ValueError:
            print("Invalid input. Please enter numerical values.")

def calculate_distance(lat1, lon1, lat2, lon2):
    R = 6371.0
    lat1, lon1, lat2, lon2 = map(radians, [lat1, lon1, lat2, lon2])
    dlat = lat2 - lat1
    dlon = lon2 - lon1
    a = sin(dlat/2)**2 + cos(lat1)*cos(lat2)*sin(dlon/2)**2
    c = 2 * atan2(sqrt(a), sqrt(1-a))
    return R * c

def fetch_latest_data(sheet):
    expected_headers = PD_EXPECTED_HEADERS if sheet.title == sheets_info['pre4'].title else WS_EXPECTED_HEADERS
    try:
        records = sheet.get_all_records(expected_headers=expected_headers)
        return records[-1] if records else None
    except Exception as e:
        print(f"Error fetching data from {sheet.title}: {e}")
        return None

def calculate_idw_prediction(latest_data, distances, param):
    weighted_sum = 0
    weight_sum = 0
    for location, distance in distances.items():
        value = latest_data[location][param]
        weight = 1 / (distance ** p)
        weighted_sum += value * weight
        weight_sum += weight
    return round(weighted_sum / weight_sum, 2)

def predict_categorical(latest_data, distances, param):
    weights = {}
    for location, distance in distances.items():
        category = latest_data[location][param]
        weight = 1 / (distance ** p)
        weights[category] = weights.get(category, 0) + weight
    return max(weights, key=weights.get)

def get_local_time():
    utc_time = datetime.now(timezone.utc)
    local_time = utc_time.astimezone(local_tz)
    return local_time.strftime("%Y-%m-%d %H:%M:%S")

def update_predictions():
    target_location = get_user_location()

    while True:
        current_time = get_local_time()
        latest_data = {}
        distances = {}

        for location, info in stations_info.items():
            distances[location] = calculate_distance(
                info['latitude'], info['longitude'],
                target_location['latitude'], target_location['longitude']
            )

        success = False
        while not success:
            print(f"Fetching data at {current_time}...")

            data_ready = True
            for location, sheet in sheets_info.items():
                if location == 'pre4':
                    continue
                latest_entry = fetch_latest_data(sheet)
                if latest_entry:
                    try:
                        latest_data[location] = {
                            'Temperature': float(latest_entry['Temperature'].replace('C', '').strip()),
                            'Humidity': float(latest_entry['Humidity'].replace('%', '').strip()),
                            'Air Pressure': float(latest_entry['Air Pressure'].replace('hPa', '').strip()),
                            'Air Quality': latest_entry['Air Quality'].strip(),
                            'Rain Status': latest_entry['Rain Status'].strip()
                        }
                    except Exception as e:
                        print(f"[{location}] Data format issue: {e}. Retrying in 10 seconds...")
                        data_ready = False
                        break
                else:
                    print(f"[{location}] No data available. Retrying in 10 seconds...")
                    data_ready = False
                    break

            if data_ready:
                success = True
            else:
                time.sleep(10)

        predictions = {
            'Temperature': calculate_idw_prediction(latest_data, distances, 'Temperature'),
            'Humidity': calculate_idw_prediction(latest_data, distances, 'Humidity'),
            'Air Pressure': calculate_idw_prediction(latest_data, distances, 'Air Pressure'),
            'Air Quality': predict_categorical(latest_data, distances, 'Air Quality'),
            'Rain Status': predict_categorical(latest_data, distances, 'Rain Status')
        }

        pd_sheet = sheets_info['pre4']
        date_str, time_str = get_local_time().split(" ")
        new_row = [
            date_str, time_str,
            target_location['latitude'], target_location['longitude'],
            predictions['Temperature'], predictions['Humidity'], predictions['Air Pressure'],
            predictions['Air Quality'], predictions['Rain Status']
        ]
        pd_sheet.append_row(new_row)

        print(f"\n✅ Predictions updated at {current_time} for location ({target_location['latitude']}, {target_location['longitude']}):")
        for key, value in predictions.items():
            print(f"   - {key}: {value}")

        time.sleep(35)

# Start the prediction loop
try:
    update_predictions()
except KeyboardInterrupt:
    print("\nPrediction process manually stopped.")


# In[ ]:




