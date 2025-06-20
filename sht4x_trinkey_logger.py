import time
import csv
import serial
import serial.tools.list_ports

# Configuration
BAUD_RATE = 115200
BASE_CSV_FILE_PATH = 'sensor_readings'
SENSOR_READ_INTERVAL = 10  # seconds

assert type(SENSOR_READ_INTERVAL) is int, "SENSOR_READ_INTERVAL must be an integer."
assert SENSOR_READ_INTERVAL > 0, "SENSOR_READ_INTERVAL must be greater than 0."

def create_file_name(base_path):
    """Create a timestamped CSV filename."""
    timestamp_str = time.strftime("%Y%m%d_%H%M%S")
    return f"{base_path}_{timestamp_str}.csv"

def parse_sensor_line(line):
    """Parse a sensor data line into its components."""
    parts = [x.strip() for x in line.split(',')]
    if len(parts) != 4:
        return None
    serial_number, timestamp, temperature, humidity = parts
    return serial_number, int(timestamp), float(temperature), float(humidity)

def get_adafruit_ports():
    """Find all Adafruit devices connected to the system."""
    ports = serial.tools.list_ports.comports()
    return [port for port in ports if port.vid == 0x239A]

def get_serial_number(ser):
    """Read the serial number from the device."""
    try:
        ser.write(b'n')
        time.sleep(0.1)
        return ser.readline().decode('utf-8').strip()
    except Exception as e:
        print(f"Error reading serial number: {e}")
        return None

def create_header(serial_handles):
    """Create a CSV header based on serial numbers."""
    header = ["timestamp"]
    for _, _, serial_number in serial_handles:
        header.append(f"{serial_number}_temperature (degrees C)")
        header.append(f"{serial_number}_humidity (% rH)")
    return header

def empty_serial_buffer(ser):
    """Flush and return all lines currently in the serial buffer."""
    result = ""
    while ser.in_waiting:
        result += ("  " + ser.readline().decode('utf-8'))
    return result

def open_serial_ports(adafruit_ports):
    """Open all serial ports and return handles with serial numbers."""
    serial_handles = []
    for port in adafruit_ports:
        try:
            ser = serial.Serial(port.device, BAUD_RATE, timeout=0.1)
            time.sleep(0.1)
            print(f"Message from {port.device}:\n{empty_serial_buffer(ser)}")
            serial_number = get_serial_number(ser)
            if not serial_number:
                print(f"Could not read serial number from {port.device}.")
                ser.close()
                continue
            serial_handles.append((port, ser, serial_number))
            print(f"Opened {port.device}, serial number: {serial_number}")
        except Exception as e:
            print(f"Could not open {port.device}: {e}")
    return serial_handles

def write_csv_header(csv_file_path, header):
    """Write the CSV header to the file."""
    with open(csv_file_path, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(header)
    print(f"CSV header: {header}, length: {len(header)}")

def request_sensor_stream(serial_handles):
    """Send 's' to all sensors to start streaming."""
    for port, ser, _ in serial_handles:
        ser.write(f's{SENSOR_READ_INTERVAL}'.encode("ascii"))
        time.sleep(0.1)
        print(f"Message from {port.device}:\n{empty_serial_buffer(ser)}")

def log_sensor_data(serial_handles, header, csv_file_path):
    """Continuously log sensor data to CSV."""
    print(f"Starting data logging to {csv_file_path}... Press Ctrl+C to stop.")
    with open(csv_file_path, mode='a', newline='') as file:
        writer = csv.writer(file)
        while True:
            for i, (port, ser, serial_number) in enumerate(serial_handles):
                row = [None] * len(header)
                if ser.in_waiting:
                    try:
                        line = ser.readline().decode('utf-8').strip()
                        if not line:
                            continue
                        if line.startswith('#'):
                            print(f"{port.device}: Comment line: {line}")
                            continue
                        parsed = parse_sensor_line(line)
                        if not parsed:
                            print(f"{port.device}: Malformed line: {line}")
                            continue
                        _, timestamp, temperature, humidity = parsed
                        row[0] = timestamp
                        row[i*2+1:i*2+3] = [temperature, humidity]
                        writer.writerow(row)
                        print(f"{port.device}: Logged: {row}")
                    except Exception as e:
                        print(f"{port.device}: Error: {e}")
            time.sleep(0.05)

def close_serial_ports(serial_handles):
    """Close all serial ports."""
    for _, ser, _ in serial_handles:
        ser.close()

def main():
    print("Searching for Adafruit devices...")
    adafruit_ports = get_adafruit_ports()
    if not adafruit_ports:
        print("No Adafruit devices found.")
        return

    csv_file_path = create_file_name(BASE_CSV_FILE_PATH)
    serial_handles = open_serial_ports(adafruit_ports)
    if not serial_handles:
        print("No serial ports could be opened.")
        return

    header = create_header(serial_handles)
    write_csv_header(csv_file_path, header)
    request_sensor_stream(serial_handles)
    try:
        log_sensor_data(serial_handles, header, csv_file_path)
    except KeyboardInterrupt:
        print("Data logging interrupted.")
    finally:
        close_serial_ports(serial_handles)

if __name__ == '__main__':
    main()
