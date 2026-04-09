#include <Wire.h>
#include <Adafruit_BMP280.h>

/*
Branchement
┌────────┬────────────────────┐
│ HW-611 │       T-Beam       │
├────────┼────────────────────┤
│ VCC    │ 3.3V               │
├────────┼────────────────────┤
│ GND    │ GND                │                                                                                                                                                                           
├────────┼────────────────────┤
│ SDA    │ GPIO 21            │                                                                                                                                                                           
├────────┼────────────────────┤
│ SCL    │ GPIO 22            │
├────────┼────────────────────┤
│ CSB    │ 3.3V ← critique    │
├────────┼────────────────────┤                                                                                                                                                                           
│ SDD    │ GND (adresse 0x76) │
└────────┴────────────────────┘
*/
Adafruit_BMP280 bmp;

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);

    if (!bmp.begin(0x76)) {
        Serial.println("BMP280 not found !");
        while (1);
    }

    Serial.println("BMP280 OK");
}

void loop() {
    Serial.print("Temp: ");
    Serial.print(bmp.readTemperature());
    Serial.println(" °C");

    Serial.print("Pressure: ");
    Serial.print(bmp.readPressure() / 100.0);
    Serial.println(" hPa");

    Serial.println();
    delay(2000);
}

// #include <Arduino.h>
// #include <Wire.h>

// void setup()
// {

//     Serial.begin(115200);
//     delay(2000);
//     Wire.begin(21,22);

//     Serial.println("Scanning...");

//     for (byte address = 1; address < 127; address++)
//     {
//         Serial.println(address);
//         Wire.beginTransmission(address);
//         if (Wire.endTransmission() == 0)
//         {
//             Serial.print("Found I2C at 0x");
//             Serial.println(address, HEX);
//         }
//     }

//     Serial.println("Done");
// }

// void loop() {}