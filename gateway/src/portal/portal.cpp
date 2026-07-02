#include "portal.h"

#include <WebServer.h>
#include <WiFi.h>
#include <cstring>

namespace {

constexpr const char *AP_SSID = "SensorSensei-Setup";
constexpr const char *AP_PASSWORD = "sensor-sensei";

WebServer server(80);

struct PortalState {
    bool apReady = false;
    bool staConnected = false;
    bool loraReady = false;
    unsigned long lastPacketMs = 0;
    char apIp[16] = "0.0.0.0";
    char staSsid[33] = "-";
    char staIp[16] = "0.0.0.0";
    char staMessage[64] = "Not connected";
    char loraMessage[64] = "Not ready";
    char lastPacketSummary[192] = "No packet received yet";
};

PortalState state;

static void copyString(char *dst, size_t dstSize, const char *src) {
    if (dstSize == 0) return;
    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

static void formatIp(char *dst, size_t dstSize, const IPAddress &ip) {
    snprintf(dst, dstSize, "%u.%u.%u.%u",
             ip[0], ip[1], ip[2], ip[3]);
}

static const char *htmlBool(bool value) {
    return value ? "yes" : "no";
}

static const char *jsonBool(bool value) {
    return value ? "true" : "false";
}

static void handleRoot() {
    char html[4096];
    snprintf(html, sizeof(html),
        "<!doctype html><html><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>SensorSensei Gateway</title>"
        "<style>"
        "body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:24px;}"
        ".card{max-width:900px;margin:0 auto;background:#111827;border:1px solid #334155;border-radius:16px;padding:24px;}"
        "h1{margin-top:0;}"
        ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:16px;}"
        ".box{background:#0b1220;border:1px solid #334155;border-radius:12px;padding:16px;}"
        ".label{color:#94a3b8;font-size:12px;text-transform:uppercase;letter-spacing:.08em;}"
        ".value{font-size:18px;margin-top:6px;word-break:break-word;}"
        "code{font-family:Consolas,monospace;}"
        "</style></head><body><div class='card'>"
        "<h1>SensorSensei Gateway</h1>"
        "<p>AP: <code>%s</code> | AP IP: <code>%s</code></p>"
        "<div class='grid'>"
        "<div class='box'><div class='label'>Station WiFi</div><div class='value'>%s</div><div class='label'>SSID</div><div class='value'><code>%s</code></div><div class='label'>IP</div><div class='value'><code>%s</code></div><div class='label'>Message</div><div class='value'>%s</div></div>"
        "<div class='box'><div class='label'>LoRa</div><div class='value'>%s</div><div class='label'>Status</div><div class='value'>%s</div></div>"
        "<div class='box'><div class='label'>Last packet</div><div class='value'>%s</div></div>"
        "</div><p style='color:#94a3b8;margin-top:16px;'>Configuration forms will be available in the next iteration.</p></div></body></html>",
        AP_SSID, state.apIp,
        htmlBool(state.staConnected), state.staSsid, state.staIp, state.staMessage,
        htmlBool(state.loraReady), state.loraMessage,
        state.lastPacketSummary
    );
    server.send(200, "text/html; charset=utf-8", html);
}

static void handleStatusJson() {
    char json[1024];
    snprintf(json, sizeof(json),
        "{"
        "\"ap_ready\":%s,"
        "\"ap_ip\":\"%s\","
        "\"wifi_connected\":%s,"
        "\"wifi_ssid\":\"%s\","
        "\"wifi_ip\":\"%s\","
        "\"lora_ready\":%s,"
        "\"lora_message\":\"%s\","
        "\"last_packet\":\"%s\""
        "}",
        jsonBool(state.apReady),
        state.apIp,
        jsonBool(state.staConnected),
        state.staSsid,
        state.staIp,
        jsonBool(state.loraReady),
        state.loraMessage,
        state.lastPacketSummary
    );
    server.send(200, "application/json; charset=utf-8", json);
}

}  // namespace

bool portalInit() {
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
        Serial.println("[AP] SoftAP failed");
        return false;
    }

    formatIp(state.apIp, sizeof(state.apIp), WiFi.softAPIP());
    state.apReady = true;

    server.on("/", HTTP_GET, handleRoot);
    server.on("/status.json", HTTP_GET, handleStatusJson);
    server.begin();

    Serial.printf("[AP] %s ready @ %s\n", AP_SSID, state.apIp);
    return true;
}

void portalLoop() {
    server.handleClient();
}

void portalSetStationStatus(bool connected, const char *ssid, const IPAddress &ip,
                            const char *message) {
    state.staConnected = connected;
    copyString(state.staSsid, sizeof(state.staSsid), ssid != nullptr && ssid[0] != '\0' ? ssid : "-");
    formatIp(state.staIp, sizeof(state.staIp), ip);
    copyString(state.staMessage, sizeof(state.staMessage), message);
}

void portalSetLoraStatus(bool ready, const char *message) {
    state.loraReady = ready;
    copyString(state.loraMessage, sizeof(state.loraMessage), message);
}

void portalSetLastPacket(const SensorPayload &p, float rssi, float snr) {
    state.lastPacketMs = millis();
    snprintf(state.lastPacketSummary, sizeof(state.lastPacketSummary),
             "Device 0x%08X | Temp %.2f C | Pressure %.0f hPa | PM2.5 %.1f ug/m3 | RSSI %.1f dBm | SNR %.1f dB | %lu ms ago",
             p.device_id, p.temperature, p.pressure, p.pm25, rssi, snr, 0UL);
    snprintf(state.lastPacketSummary, sizeof(state.lastPacketSummary),
             "Device 0x%08X | Temp %.2f C | Pressure %.0f hPa | PM2.5 %.1f ug/m3 | RSSI %.1f dBm | SNR %.1f dB | now",
             p.device_id, p.temperature, p.pressure, p.pm25, rssi, snr);
}
