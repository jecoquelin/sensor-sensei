#include <Arduino.h>
#include "sensors/bmp280.h"
#include "sensors/mic.h"
#include "lora/transmitter.h"
#include "../../shared/payload.h"

#define DEVICE_ID   0x0001
#define SLEEP_SEC   1

void setup() {
    Serial.begin(115200);

    // if (!bmp280Init()) while (1);
    if (!micInit())    while (1);
    if (!loraInit())   while (1);
}

void loop() {
    SensorPayload p;
    p.device_id   = DEVICE_ID;
    // p.temperature = bmp280ReadTemperature();
    // p.pressure    = bmp280ReadPressure();
    p.noise_db    = micReadDb();

    Serial.printf("Bruit: %.1f dB\n", p.noise_db);
    // Serial.printf("[Node] Temp: %.2f °C | Pres: %.0f hPa | Bruit: %.1f dB\n",
    //               p.temperature, p.pressure, p.noise_db);

    uint8_t buf[PAYLOAD_LEN];
    encodePayload(buf, p);
    loraSend(buf, PAYLOAD_LEN);

    delay(SLEEP_SEC * 1000);
}
