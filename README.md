# SHT4x Trinkey Logger

## Arduino Code: `arduino\adafruit-sht4x-trinkey\adafruit-sht4x-trinkey.ino`

### Overview
This sketch runs on an Adafruit SHT4x sensor board (e.g., SHT41 Trinkey) and communicates via USB serial. It waits for user commands to either print the sensor’s serial number or start streaming temperature and humidity readings.

### Features

- **Serial Command Interface:**
  - Send `'n'` to print the sensor’s serial number.
  - Send `'s'` to start continuous measurement output.

- **Sensor Output:**
  - Outputs lines in the format:  
    `serial_number_of_sht41, timestamp, temperature (C), humidity (% rH)`

- **NeoPixel Status LED:**
  - **Blue:** Startup
  - **White:** Ready for command
  - **Magenta:** Sending data through serial
  - **Green:** Successful read, waiting for next read
  - **Yellow:** Error reading sensor

### Usage

1. Upload the sketch to your SHT4x board.
2. Open a serial terminal at 115200 baud.
3. Follow on-screen instructions:
    - Send `'n'` to get the serial number.
    - Send `'s'` to start data streaming.
4. Each reading is output as a CSV line.

---

## Python Logger: `sht4x_trinkey_logger.py`

### Overview
This script runs on your PC and logs data from one or more SHT4x sensor boards connected via USB. It auto-detects Adafruit devices, requests data streaming, and saves readings to a timestamped CSV file.

### Features

- **Auto-Detects Devices:**  
  Finds all connected Adafruit boards using their USB vendor ID.
- **Serial Number Identification:**  
  Reads each device’s serial number for unique identification.
- **CSV Logging:**  
  Creates a CSV file with columns for each device’s temperature and humidity.
- **Command Automation:**  
  Sends `'s'` to each device to start data streaming.
- **Robust Parsing:**  
  Ignores comments and malformed lines; prints errors for debugging.

### Usage

1. Connect one or more SHT4x boards to your PC.
2. Run the script:
    ```sh
    python sht4x_trinkey_logger.py
    ```
3. The script will:
    - Detect devices
    - Query serial numbers
    - Start data streaming
    - Log all readings to a CSV file named like `sensor_readings_YYYYMMDD_HHMMSS.csv`
4. Stop logging with `Ctrl+C`.

### CSV Output Format

- **Columns:**  
  `timestamp, <serial>_temperature (degrees C), <serial>_humidity (% rH), ...`
- Each row contains readings from all connected devices, aligned by timestamp.

---

## Summary

- The Arduino code streams sensor data over serial after receiving a command.
- The Python script automates device detection, starts data streaming, and logs all readings to a CSV file for later analysis.