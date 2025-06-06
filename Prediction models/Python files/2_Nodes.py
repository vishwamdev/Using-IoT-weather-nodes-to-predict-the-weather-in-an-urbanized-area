#!/usr/bin/env python
# coding: utf-8

# In[2]:


import gspread
from oauth2client.service_account import ServiceAccountCredentials
import time
from datetime import datetime, timezone
import pytz
from math import radians, sin, cos, sqrt, atan2  # For distance calculation

# Define weather station coordinates (only two)
stations_info = {
    'WS1': {'latitude': 7.0193689, 'longitude': 79.9001577},
    'WS2': {'latitude': 7.0193110, 'longitude': 79.9002777}
}

# Power parameter for IDW
p = 2

# Google Sheets credentials and access
scope = ["https://spreadsheets.google.com/feeds", "https://www.googleapis.com/auth/drive"]
creds = ServiceAccountCredentials.from_json_keyfile_name("your_google_credentials1.json", scope)
client = gspread.authorize(creds)

# Google Sheets references (only two + pre2)
sheets_info = {
    'WS1': client.open("WS1").sheet1,
    'WS2': client.open("WS2").sheet1,
    'pre2': client.open("pre2").sheet1  
}

# Set your local timezone
local_tz = pytz.timezone('Asia/Colombo')

# Expected headers
WS_EXPECTED_HEADERS = ['Date', 'Time', 'Temperature', 'Humidity', 'Air Pressure', 'Air Quality', 'Rain Status']
PD_EXPECTED_HEADERS = ['Date', 'Time', 'Latitude', 'Longitude', 'Temperature', 'Humidity', 'Air Pressure', 'Air Quality', 'Rain Status']

# Get user input
def get_user_location():
    while True:
        try:
            lat = float(input("Enter the latitude of the target location: "))
            lon = float(input("Enter the longitude of the target location: "))
            if -90 <= lat <= 90 and -180 <= lon <= 180:
                return {'latitude': lat, 'longitude': lon}
            else:
                print("Invalid latitude/longitude. Please enter valid coordinates.")
        except ValueError:
            print("Invalid input. Please enter numerical values.")

# Calculate distance using Haversine formula
def calculate_distance(lat1, lon1, lat2, lon2):
    R = 6371.0  # Earth radius in km
    lat1, lon1, lat2, lon2 = map(radians, [lat1, lon1, lat2, lon2])
    dlat = lat2 - lat1
    dlon = lon2 - lon1
    a = sin(dlat / 2)**2 + cos(lat1) * cos(lat2) * sin(dlon / 2)**2
    c = 2 * atan2(sqrt(a), sqrt(1 - a))
    return R * c

# Fetch latest data
def fetch_latest_data(sheet):
    expected_headers = PD_EXPECTED_HEADERS if sheet.title == sheets_info['pre2'].title else WS_EXPECTED_HEADERS
    try:
        records = sheet.get_all_records(expected_headers=expected_headers)
        return records[-1] if records else None
    except Exception as e:
        print(f"Error fetching latest data from {sheet.title}: {e}")
        return None

# IDW for numerical values
def calculate_idw_prediction(latest_data, distances, param):
    weighted_sum = 0
    weight_sum = 0
    for location, distance in distances.items():
        value = latest_data[location][param]
        weight = 1 / (distance ** p)
        weighted_sum += value * weight
        weight_sum += weight
    return round(weighted_sum / weight_sum, 2)

# Categorical value prediction (majority vote with weights)
def predict_categorical(latest_data, distances, param):
    weights = {}
    for location, distance in distances.items():
        category = latest_data[location][param]
        weight = 1 / (distance ** p)
        weights[category] = weights.get(category, 0) + weight
    return max(weights, key=weights.get)

# Get local time
def get_local_time():
    utc_time = datetime.now(timezone.utc)
    local_time = utc_time.astimezone(local_tz)
    return local_time.strftime("%Y-%m-%d %H:%M:%S")

# Main update loop
def update_predictions():
    target_location = get_user_location()

    while True:
        current_time = get_local_time()
        latest_data = {}
        distances = {}

        # Calculate distances
        for location, info in stations_info.items():
            distances[location] = calculate_distance(
                info['latitude'], info['longitude'],
                target_location['latitude'], target_location['longitude']
            )

        # Fetch latest sensor data
        data_ready = True
        for location, sheet in sheets_info.items():
            if location == 'pre2':
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
                except (ValueError, KeyError) as e:
                    print(f"[{location}] Data unreadable: {e}. Waiting for valid data...")
                    data_ready = False
                    break
            else:
                print(f"[{location}] No data found. Waiting for next update...")
                data_ready = False
                break

        if not data_ready:
            time.sleep(10)
            continue

        # Generate predictions
        predictions = {
            'Temperature': calculate_idw_prediction(latest_data, distances, 'Temperature'),
            'Humidity': calculate_idw_prediction(latest_data, distances, 'Humidity'),
            'Air Pressure': calculate_idw_prediction(latest_data, distances, 'Air Pressure'),
            'Air Quality': predict_categorical(latest_data, distances, 'Air Quality'),
            'Rain Status': predict_categorical(latest_data, distances, 'Rain Status')
        }

        # Write prediction to sheet
        pd_sheet = sheets_info['pre2']
        local_date, local_time_str = get_local_time().split(" ")
        new_row = [
            local_date, local_time_str,
            target_location['latitude'], target_location['longitude'],
            predictions['Temperature'], predictions['Humidity'], predictions['Air Pressure'],
            predictions['Air Quality'], predictions['Rain Status']
        ]
        pd_sheet.append_row(new_row)

        print(f"✅ Updated Predictions for Location ({target_location['latitude']}, {target_location['longitude']}) at {current_time}: {predictions}")
        time.sleep(35)

# Start predictions
try:
    update_predictions()
except KeyboardInterrupt:
    print("Prediction update process stopped.")


# In[ ]:




