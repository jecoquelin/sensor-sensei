#include <Arduino.h>
#include <Wire.h>

void setup()
{

    Serial.begin(115200);
    delay(2000);
    Wire.begin(21,22);      //Pin ou le capteur est branché

    Serial.println("Scanning...");

    for (byte address = 1; address < 127; address++)
    {
        Serial.println(address);
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("Found I2C at 0x");
            Serial.println(address, HEX);
        }
    }

    Serial.println("Done");
}

void loop() {}