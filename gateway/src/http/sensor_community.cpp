#include "sensor_community.h"
#include <cstring>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino.h>

#define SC_URL "https://api.sensor.community/v1/push-sensor-data/"

// Envoie un corps JSON à sensor.community pour un pin donné.
// sensor.community identifie le type de capteur par le numéro de pin dans le header :
//   pin 1 → SDS011 / PM sensor  (P1, P2)
//   pin 3 → BMP180 / BMP280     (température, pression)
//   pin 11 → BME280             (température, pression, humidité)
static bool postPin(const char *sensor_id, const char *pin, const char *body) {
    WiFiClientSecure client;
    // setInsecure() désactive la vérification du certificat SSL.
    // Suffisant pour ce projet embarqué — pas de stockage de CA root possible.
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, SC_URL)) return false;

    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Sensor", sensor_id);  // identifiant unique de la station
    http.addHeader("X-Pin",    pin);         // type de capteur

    int code = http.POST((uint8_t *)body, strlen(body));
    // sensor.community retourne 200 ou 201 selon si c'est un nouvel enregistrement
    bool ok = (code == 200 || code == 201);
    if (ok)
        Serial.printf("[SC] pin %s → OK (%d)\n", pin, code);
    else
        Serial.printf("[SC] pin %s → erreur %d : %s\n", pin, code, http.getString().c_str());
    http.end();
    return ok;
}

bool scSend(const SensorPayload &p) {
    // L'identifiant de station sensor.community est "esp32-<device_id_decimal>"
    char sensor_id[20];
    snprintf(sensor_id, sizeof(sensor_id), "esp32-%u", p.device_id);

    // ── BMP280 → pin 3 ───────────────────────────────────────────────────────
    // Le BMP280 ne mesure pas l'humidité. On envoie uniquement température + pression.
    // La pression doit être envoyée en Pa (pascals), pas en hPa.
    // sensor.community affiche ensuite la valeur convertie en hPa dans l'interface.
    char bmp_body[220];
    snprintf(bmp_body, sizeof(bmp_body),
        "{\"software_version\":\"SensorSensei-1.0\","
        "\"sensordatavalues\":["
        "{\"value_type\":\"temperature\",\"value\":\"%.2f\"},"
        "{\"value_type\":\"pressure\",\"value\":\"%.0f\"}"
        "]}",
        p.temperature,
        p.pressure * 100.0f  // hPa → Pa
    );

    // ── Dust sensor → pin 1 (format SDS011) ──────────────────────────────────
    // Le GP2Y1010 renvoie directement des µg/m³, compatible avec le format SDS011
    // qui attend P1 (PM10) et P2 (PM2.5) en µg/m³. On envoie la même valeur pour les deux
    // car ce capteur ne distingue pas les tailles de particules.
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
    Serial.printf("[Pin 3]  %s\n", bmp_body);
    Serial.printf("[Pin 1]  %s\n", dust_body);
    Serial.println("────────────────────────────────");

    bool ok = postPin(sensor_id, "3", bmp_body);
    ok &= postPin(sensor_id, "1",  dust_body);
    return ok;
}
