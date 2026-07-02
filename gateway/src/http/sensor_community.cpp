#include "sensor_community.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <cstring>

#define SC_URL "https://api.sensor.community/v1/push-sensor-data/"

static bool postPin(const char *sensor_id, const char *pin, const char *body) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, SC_URL)) return false;

    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Sensor", sensor_id);
    http.addHeader("X-Pin", pin);

    int code = http.POST((uint8_t *)body, strlen(body));
    bool ok = (code == 200 || code == 201);
    if (ok)
        Serial.printf("[SC] pin %s -> OK (%d)\n", pin, code);
    else
        Serial.printf("[SC] pin %s -> erreur %d : %s\n", pin, code, http.getString().c_str());
    http.end();
    return ok;
}

static void buildSensorId(char *sensor_id, size_t len, uint32_t device_id) {
    snprintf(sensor_id, len, "esp32-%u", device_id);
}

static void buildBmpBody(const SensorCommunityJob &job, char *body, size_t len) {
    snprintf(body, len,
        "{\"software_version\":\"SensorSensei-1.0\","
        "\"sensordatavalues\":["
        "{\"value_type\":\"temperature\",\"value\":\"%.2f\"},"
        "{\"value_type\":\"pressure\",\"value\":\"%.0f\"}"
        "]}",
        job.temperature_centi / 100.0f,
        job.pressure_hpa * 100.0f);
}

static void buildDustBody(const SensorCommunityJob &job, char *body, size_t len) {
    snprintf(body, len,
        "{\"software_version\":\"SensorSensei-1.0\","
        "\"sensordatavalues\":["
        "{\"value_type\":\"P1\",\"value\":\"%.1f\"},"
        "{\"value_type\":\"P2\",\"value\":\"%.1f\"}"
        "]}",
        job.pm25_deci / 10.0f,
        job.pm25_deci / 10.0f);
}

bool scSendJob(const SensorCommunityJob &job) {
    char sensor_id[20];
    buildSensorId(sensor_id, sizeof(sensor_id), job.device_id);

    char body[220];
    const char *pin = nullptr;
    if (job.pin == 11) {
        pin = "11";
        buildBmpBody(job, body, sizeof(body));
    } else if (job.pin == 1) {
        pin = "1";
        buildDustBody(job, body, sizeof(body));
    } else {
        Serial.printf("[SC] pin %u inconnu\n", job.pin);
        return false;
    }

    Serial.printf("[SC] send pin %s pour %s\n", pin, sensor_id);
    return postPin(sensor_id, pin, body);
}

bool scSend(const SensorPayload &p) {
    SensorCommunityJob bmpJob{};
    bmpJob.device_id = p.device_id;
    bmpJob.pin = 11;
    bmpJob.temperature_centi = (int16_t)(p.temperature * 100.0f);
    bmpJob.pressure_hpa = (uint16_t)(p.pressure);
    bmpJob.pm25_deci = (uint16_t)(p.pm25 * 10.0f);

    SensorCommunityJob dustJob = bmpJob;
    dustJob.pin = 1;

    bool ok = scSendJob(bmpJob);
    ok &= scSendJob(dustJob);
    return ok;
}
