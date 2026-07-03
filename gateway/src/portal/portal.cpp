#include "portal.h"

#include "gatekeeper.h"
#include "settings/settings.h"

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
    bool hasLastPacket = false;
    unsigned long lastPacketMs = 0;
    char apIp[16] = "0.0.0.0";
    char staSsid[33] = "-";
    char staIp[16] = "0.0.0.0";
    char staMessage[64] = "Not connected";
    char loraMessage[64] = "Not ready";
    char whitelistSummary[256] = "accept all nodes";
    uint32_t lastDeviceId = 0;
    float lastTemperature = 0.0f;
    float lastPressure = 0.0f;
    float lastPm25 = 0.0f;
    float lastRssi = 0.0f;
    float lastSnr = 0.0f;
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

static void formatLastPacket(char *dst, size_t dstSize) {
    if (!state.hasLastPacket) {
        strncpy(dst, "No packet received yet", dstSize - 1);
        dst[dstSize - 1] = '\0';
        return;
    }

    unsigned long ageSec = (millis() - state.lastPacketMs) / 1000UL;
    snprintf(dst, dstSize,
             "Device 0x%08X | Temp %.2f C | Pressure %.0f hPa | PM2.5 %.1f ug/m3 | RSSI %.1f dBm | SNR %.1f dB | %lu s ago",
             state.lastDeviceId, state.lastTemperature, state.lastPressure,
             state.lastPm25, state.lastRssi, state.lastSnr, ageSec);
}

static void handleRoot() {
    settingsFormatWhitelist(state.whitelistSummary, sizeof(state.whitelistSummary));

    char whitelistEdit[512];
    whitelistEdit[0] = '\0';
    const GatewaySettings &cfg = settingsGet();
    if (cfg.whitelistCount > 0) {
        size_t used = 0;
        for (size_t i = 0; i < cfg.whitelistCount; ++i) {
            int written = snprintf(whitelistEdit + used, sizeof(whitelistEdit) - used,
                                   i == 0 ? "0x%08X" : ", 0x%08X",
                                   cfg.whitelist[i]);
            if (written < 0 || static_cast<size_t>(written) >= sizeof(whitelistEdit) - used) {
                whitelistEdit[sizeof(whitelistEdit) - 1] = '\0';
                break;
            }
            used += static_cast<size_t>(written);
        }
    }

    char lastPacketText[192];
    formatLastPacket(lastPacketText, sizeof(lastPacketText));

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
        "input,textarea{width:100%%;box-sizing:border-box;margin-top:8px;padding:10px;border-radius:8px;border:1px solid #475569;background:#0f172a;color:#e2e8f0;}"
        "textarea{min-height:120px;resize:vertical;}"
        "button{margin-top:12px;padding:10px 14px;border:0;border-radius:8px;background:#38bdf8;color:#082f49;font-weight:700;cursor:pointer;}"
        "code{font-family:Consolas,monospace;}"
        "</style></head><body><div class='card'>"
        "<h1>SensorSensei Gateway</h1>"
        "<p>AP: <code>%s</code> | AP IP: <code>%s</code></p>"
        "<div class='grid'>"
        "<div class='box'><div class='label'>Station WiFi</div><div class='value'>%s</div><div class='label'>SSID</div><div class='value'><code>%s</code></div><div class='label'>IP</div><div class='value'><code>%s</code></div><div class='label'>Message</div><div class='value'>%s</div></div>"
        "<div class='box'><div class='label'>LoRa</div><div class='value'>%s</div><div class='label'>Status</div><div class='value'>%s</div></div>"
        "<div class='box'><div class='label'>Whitelist</div><div class='value'><code>%s</code></div></div>"
        "<div class='box'><div class='label'>Last packet</div><div class='value'>%s</div></div>"
        "<div class='box'><div class='label'>WiFi config</div><form method='post' action='/wifi'><input name='ssid' placeholder='SSID' value='%s'><input name='password' type='password' placeholder='Password'><button type='submit'>Save WiFi</button></form></div>"
        "<div class='box'><div class='label'>Whitelist</div><form method='post' action='/whitelist'><textarea name='whitelist' placeholder='0x12345678, 0x9ABCDEF0'>%s</textarea><button type='submit'>Save whitelist</button></form></div>"
        "</div><p style='color:#94a3b8;margin-top:16px;'>Edit WiFi or whitelist here. Saving WiFi restarts the gateway.</p></div></body></html>",
        AP_SSID, state.apIp,
        htmlBool(state.staConnected), state.staSsid, state.staIp, state.staMessage,
        htmlBool(state.loraReady), state.loraMessage,
        state.whitelistSummary,
        lastPacketText,
        settingsGet().wifiSsid,
        whitelistEdit
    );
    server.send(200, "text/html; charset=utf-8", html);
}

static void handleStatusJson() {
    settingsFormatWhitelist(state.whitelistSummary, sizeof(state.whitelistSummary));
    char lastPacketText[192];
    formatLastPacket(lastPacketText, sizeof(lastPacketText));
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
        "\"whitelist\":\"%s\","
        "\"last_packet\":\"%s\","
        "\"last_packet_age_s\":%lu,"
        "\"last_device_id\":%u,"
        "\"last_temperature\":%.2f,"
        "\"last_pressure\":%.0f,"
        "\"last_pm25\":%.1f,"
        "\"last_rssi\":%.1f,"
        "\"last_snr\":%.1f"
        "}",
        jsonBool(state.apReady),
        state.apIp,
        jsonBool(state.staConnected),
        state.staSsid,
        state.staIp,
        jsonBool(state.loraReady),
        state.loraMessage,
        state.whitelistSummary,
        lastPacketText,
        state.hasLastPacket ? (millis() - state.lastPacketMs) / 1000UL : 0UL,
        state.lastDeviceId,
        state.lastTemperature,
        state.lastPressure,
        state.lastPm25,
        state.lastRssi,
        state.lastSnr
    );
    server.send(200, "application/json; charset=utf-8", json);
}

static void handleWifiSave() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    ssid.trim();
    password.trim();

    if (ssid.isEmpty()) {
        server.send(400, "text/plain; charset=utf-8", "SSID is required");
        return;
    }

    if (!settingsSaveWiFi(ssid.c_str(), password.c_str())) {
        server.send(500, "text/plain; charset=utf-8", "Failed to save WiFi settings");
        return;
    }

    portalSetStationStatus(false, ssid.c_str(), WiFi.localIP(), "Saved. Restarting gateway...");
    server.send(200, "text/plain; charset=utf-8", "WiFi saved. Restarting gateway...");
    delay(1000);
    ESP.restart();
}

static void handleWhitelistSave() {
    String text = server.arg("whitelist");
    text.trim();

    uint32_t parsed[GATEWAY_MAX_WHITELIST];
    size_t parsedCount = 0;
    if (!settingsParseWhitelistText(text.c_str(), parsed, GATEWAY_MAX_WHITELIST, parsedCount)) {
        server.send(400, "text/plain; charset=utf-8", "Invalid whitelist format");
        return;
    }

    if (!settingsSaveWhitelist(parsed, parsedCount)) {
        server.send(500, "text/plain; charset=utf-8", "Failed to save whitelist");
        return;
    }

    gatekeeperSetWhitelist(parsed, parsedCount);
    settingsFormatWhitelist(state.whitelistSummary, sizeof(state.whitelistSummary));
    server.send(200, "text/plain; charset=utf-8", "Whitelist saved");
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
    settingsFormatWhitelist(state.whitelistSummary, sizeof(state.whitelistSummary));

    server.on("/", HTTP_GET, handleRoot);
    server.on("/status.json", HTTP_GET, handleStatusJson);
    server.on("/wifi", HTTP_POST, handleWifiSave);
    server.on("/whitelist", HTTP_POST, handleWhitelistSave);
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
    state.hasLastPacket = true;
    state.lastDeviceId = p.device_id;
    state.lastTemperature = p.temperature;
    state.lastPressure = p.pressure;
    state.lastPm25 = p.pm25;
    state.lastRssi = rssi;
    state.lastSnr = snr;
}
