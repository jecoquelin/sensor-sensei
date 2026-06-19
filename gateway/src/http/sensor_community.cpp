#include "sensor_community.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino.h>

#define SC_URL "https://api.sensor.community/v1/push-sensor-data/"

static bool postPin(const char *sensor_id, const char *pin, const char *body) {
    WiFiClientSecure client;
    client.setInsecure();  // pas de vérif certificat, suffisant pour ce projet

    HTTPClient http;
    if (!http.begin(client, SC_URL)) {
        Serial.printf("[SC] begin() failed (pin %s)\n", pin);
        return false;
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Sensor", sensor_id);
    http.addHeader("X-Pin", pin);

    int code = http.POST((uint8_t *)body, strlen(body));
    bool ok = (code == 200 || code == 201);
    if (ok)
        Serial.printf("[SC] pin %s → OK (200)\n", pin);
    else
        Serial.printf("[SC] pin %s → erreur %d : %s\n", pin, code, http.getString().c_str());
    http.end();
    return ok;
}

bool scSend(const SensorPayload &p) {
    char sensor_id[20];
    snprintf(sensor_id, sizeof(sensor_id), "esp32-%u", p.device_id);

    // ── BMP280 : température + pression (pin 11) ─────────────────────────────
    char bmp_body[220];
    snprintf(bmp_body, sizeof(bmp_body),
        "{\"software_version\":\"SensorSensei-1.0\","
        "\"sensordatavalues\":["
        "{\"value_type\":\"temperature\",\"value\":\"%.2f\"},"
        "{\"value_type\":\"pressure\",\"value\":\"%.0f\"}"
        "]}",
        p.temperature,
        p.pressure * 100.0f  // hPa → Pa, attendu par sensor.community
    );

    // ── Dust sensor : PM2.5 (pin 3 = PPD42NS) ───────────────────────────────
    char dust_body[200];
    snprintf(dust_body, sizeof(dust_body),
        "{\"software_version\":\"SensorSensei-1.0\","
        "\"sensordatavalues\":["
        "{\"value_type\":\"P1\",\"value\":\"%.1f\"},"
        "{\"value_type\":\"P2\",\"value\":\"%.1f\"}"
        "]}",
        p.pm25, p.pm25
    );

    Serial.println("─── sensor.community payload ───");
    Serial.printf("X-Sensor: %s\n", sensor_id);
    Serial.printf("[Pin 11] %s\n", bmp_body);
    Serial.printf("[Pin 1]  %s\n", dust_body);
    Serial.println("─────────────────────────────────");


    bool ok = postPin(sensor_id, "11", bmp_body);
    ok &= postPin(sensor_id, "1", dust_body);
    return ok;
}
