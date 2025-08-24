/*
 * Adafruit SHT4x Trinkey Humidity/Temperature Sensor Logger
 * 
 * This program interfaces with the SHT4x sensor on the Adafruit Trinkey
 * to log temperature and humidity readings. Features include:
 * - Serial communication for data logging
 * - NeoPixel status indication
 * - Watchdog timer for reliability
 * - Sensor decontamination heating
 * 
 * LED Status Colors:
 * - Blue: Initializing
 * - Gray: Ready/Waiting for commands
 * - Green: Decontamination mode
 * - Yellow: Error state
 * - Magenta: Taking measurement
 * - Off: Measurement complete
 */

#include <Arduino.h>
#include "Adafruit_SHT4x.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SleepyDog.h>

// Constants
#define SETUP_MSG "Send 's' to start measurement, 'n' to get serial number, 'h' for decontamination."
#define WATCHDOG_TIMEOUT_MS 60000                    // 60 second watchdog timeout
#define DEFAULT_DECONTAMINATION_MS (30 * 60 * 1000)  // 30 minutes default decontamination
#define DECONTAMINATION_STATUS_INTERVAL 5000         // Status update interval during decontamination

// LED Color Definitions
#define LED_INIT        0x0000FF  // Blue - Initializing
#define LED_READY       0x3F3F3F  // Gray - Ready/Waiting
#define LED_DECONTAM    0x00FF00  // Green - Decontamination
#define LED_ERROR       0xFFFF00  // Yellow - Error
#define LED_MEASURING   0xFF00FF  // Magenta - Taking measurement
#define LED_OFF         0x000000  // Off - Measurement complete

// Global objects
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Global variables
uint32_t sht4SerialNumber;        // Sensor serial number

/**
 * Handle sensor decontamination heating process
 * Reads optional time parameter from serial, defaults to 30 minutes
 */
void handleDecontamination() {
  delay(1000);  // Brief delay to ensure serial buffer is ready

  // Read decontamination interval from serial input
  unsigned long decontaminationUntil = 0;
  int decontaminationInterval = Serial.parseInt();
  
  if (decontaminationInterval == 0) {
    decontaminationInterval = DEFAULT_DECONTAMINATION_MS;
    Serial.println("# Invalid decontamination interval, using default (30 min)...");
  }

  // Start decontamination process
  Serial.print("# Starting ");
  Serial.print(decontaminationInterval);
  Serial.println(" ms decontamination heater...");

  decontaminationUntil = ((unsigned long) decontaminationInterval) + millis();
  sht4.setHeater(SHT4X_HIGH_HEATER_1S);
  
  // Set LED to green (decontamination mode)
  pixel.setPixelColor(0, LED_DECONTAM);
  pixel.show();
  
  // Monitor decontamination process
  sensors_event_t humidity, temp;
  while (millis() < decontaminationUntil) {
    if (!sht4.getEvent(&humidity, &temp)) {
      // Error reading sensor - abort decontamination
      pixel.setPixelColor(0, LED_ERROR);
      pixel.show();
      Serial.println("Error reading from sensor, abort...");
      break;
    } 
    
    // Display status every ~5 seconds
    long countdown = decontaminationUntil - millis();
    if (countdown % DECONTAMINATION_STATUS_INTERVAL < 1000) {
      Serial.print("Decontaminating: T=");
      Serial.print(temp.temperature);
      Serial.print("°C, RH=");
      Serial.print(humidity.relative_humidity);
      Serial.print("%, ");
      Serial.print(countdown);
      Serial.println(" ms left");
    }
  }
  
  // Decontamination complete - return to ready state
  Serial.println("# Decontamination complete");
  Serial.println(SETUP_MSG);
  pixel.setPixelColor(0, LED_READY);
  pixel.show();
  sht4.setHeater(SHT4X_NO_HEATER);
}

/**
 * Setup function - Initialize hardware and wait for user commands
 */
void setup() {
  // Initialize NeoPixel and set to blue (initializing)
  pixel.begin();
  pixel.setPixelColor(0, LED_INIT);
  pixel.show();
  
  // Initialize serial communication at 115200 baud
  Serial.begin(115200);
  while (!Serial) {
    delay(10);     // Wait for serial console to open (Zero, Leonardo, etc.)
  }

  // Initialize and verify SHT4x sensor
  Serial.println("# Adafruit SHT41");
  if (!sht4.begin()) {
    Serial.println("# Couldn't find SHT4x");
    while (1) delay(1);  // Halt execution if sensor not found
  }
  
  // Read and display sensor serial number
  Serial.println("# Found SHT4x sensor");
  Serial.print("# Serial number: 0x");
  sht4SerialNumber = sht4.readSerial();
  Serial.println(sht4SerialNumber, HEX);

  // Display available commands
  Serial.println(SETUP_MSG);

  // Set LED to gray (ready state)
  pixel.setPixelColor(0, LED_READY);
  pixel.show();
  
  // Command processing loop - wait for user input
  while (1) {
    if (!Serial.available()) {
      delay(10);
      continue;
    }
    
    char input = Serial.read();
    
    if (input == 'n') {
      // Display sensor serial number
      Serial.print("0x");
      Serial.println(sht4SerialNumber, HEX);
      
    } else if (input == 's') {
      // Start measurement mode with watchdog enabled
      int countdownMS = Watchdog.enable(WATCHDOG_TIMEOUT_MS);
      Serial.print("Enabled the watchdog with max countdown of ");
      Serial.print(countdownMS);
      Serial.println(" milliseconds!");
      break;  // Exit command loop and proceed to measurement mode
      
    } else if (input == 'h') {
      // Sensor decontamination mode
      handleDecontamination();
      
    } else {
      // Unknown command - display help
      Serial.println(SETUP_MSG);
    }
  }

  // Configure sensor for high precision measurements
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  // Print CSV header for data logging
  Serial.println("#=========================#");
  Serial.println("# sht4SerialNumber, timestamp, temperature (degrees C), humidity (% rH)");
}

/**
 * Main loop - Wait for 'u' command to take a measurement
 * Outputs CSV format: serial_number, timestamp, temperature, humidity
 */
void loop() {
  // Wait for serial input
  while (!Serial.available()) {
    delay(10);
  }

  char input = Serial.read();

  // Take measurement on 'u' command
  if (input == 'u') {
    sensors_event_t humidity, temp;
    
    if (sht4.getEvent(&humidity, &temp)) {
      // Successful measurement - indicate with magenta LED
      pixel.setPixelColor(0, LED_MEASURING);
      pixel.show();
      
      // Output data in CSV format
      Serial.print("0x");
      Serial.print(sht4SerialNumber, HEX);
      Serial.print(", ");
      Serial.print(millis());
      Serial.print(", ");
      Serial.print(temp.temperature);
      Serial.print(", ");
      Serial.println(humidity.relative_humidity);
      
      // Turn off LED and reset watchdog
      pixel.setPixelColor(0, LED_OFF);
      pixel.show();
      Watchdog.reset();
      
    } else {
      // Error reading sensor - indicate with yellow LED
      pixel.setPixelColor(0, LED_ERROR);
      pixel.show();
      Serial.println("Error reading from sensor, retrying...");
    }
  }
  // Note: Other commands are ignored in measurement mode
}
