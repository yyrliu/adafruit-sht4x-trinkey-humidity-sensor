/*************************************************** 
  This is an example for the SHT4x Humidity & Temp Sensor

  Designed specifically to work with the SHT4x sensor from Adafruit
  ----> https://www.adafruit.com/products/4885

  These sensors use I2C to communicate, 2 pins are required to  
  interface
 ****************************************************/

#include "Adafruit_SHT4x.h"
#include <Adafruit_NeoPixel.h>

#define SETUP_MSG "Send 's' to start measurement, 'n' to get serial number."

Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

int sampleInterval = 1000;
uint32_t sht4SerialNumber;

void setup() {
  pixel.begin();
  pixel.setPixelColor(0, 0x0000FF);
  pixel.show();
  Serial.begin(115200);

  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("# Adafruit SHT41");
  if (! sht4.begin()) {
    Serial.println("# Couldn't find SHT4x");
    while (1) delay(1);
  }
  Serial.println("# Found SHT4x sensor");
  Serial.print("# Serial number: 0x");
  sht4SerialNumber = sht4.readSerial();
  Serial.println(sht4SerialNumber, HEX);

  Serial.println(SETUP_MSG);

  pixel.setPixelColor(0, 0x3F3F3F);
  pixel.show();
  
  while (1) {
    if (!Serial.available()) {
      delay(10);
      continue;
    } 
    
    char input = Serial.read();
    
    if (input == 'n') {
      Serial.print("0x");
      Serial.println(sht4SerialNumber, HEX);
    } else if (input == 's') {
      delay(100);
      int sampleIntervalInSec = Serial.parseInt();
      if (sampleIntervalInSec == 0) {
        Serial.println("# Invalid sampling interval, using default 1000 ms");
      } else {
        sampleInterval = sampleIntervalInSec * 1000;
        Serial.print("# Sampling interval set to ");
        Serial.print(sampleInterval);
        Serial.println(" ms");
      }
      Serial.println("# Measuring...");
      break;
    } else {
      Serial.println(SETUP_MSG);
    }
  }

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  Serial.println("#=========================#");
  Serial.println("# sht4SerialNumber, timestamp, temperature (degrees C), humidity (% rH)");
}


void loop() {
  
  sensors_event_t humidity, temp;

  if (sht4.getEvent(&humidity, &temp)) {
    pixel.setPixelColor(0, 0xFF00FF);
    pixel.show();
    Serial.print("0x");
    Serial.print(sht4SerialNumber, HEX);
    Serial.print(", ");
    Serial.print(millis());
    Serial.print(", ");
    Serial.print(temp.temperature);
    Serial.print(", ");
    Serial.println(humidity.relative_humidity);
    pixel.setPixelColor(0, 0x000000);
    pixel.show();
  } else {
    pixel.setPixelColor(0, 0xFFFF00);
    pixel.show();
    Serial.println("Error reading from sensor, retrying...");
  }

  delay(sampleInterval);
}
