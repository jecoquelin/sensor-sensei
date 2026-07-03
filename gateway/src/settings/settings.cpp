#include "settings.h"

#include <Preferences.h>
#include <cstring>
#include <cstdlib>

namespace {

constexpr const char *SETTINGS_NAMESPACE = "sensor-sensei";
constexpr const char *SETTINGS_KEY = "gateway";
constexpr uint8_t SETTINGS_VERSION = 1;

struct StoredSettings {
    uint8_t version;
    char wifiSsid[WIFI_SSID_LEN];
    char wifiPassword[WIFI_PASSWORD_LEN];
    uint32_t whitelist[GATEWAY_MAX_WHITELIST];
    uint8_t whitelistCount;
};

GatewaySettings state;
bool loaded = false;

static void resetState() {
    memset(&state, 0, sizeof(state));
}

static bool saveState() {
    StoredSettings stored{};
    stored.version = SETTINGS_VERSION;
    memcpy(stored.wifiSsid, state.wifiSsid, sizeof(stored.wifiSsid));
    memcpy(stored.wifiPassword, state.wifiPassword, sizeof(stored.wifiPassword));
    memcpy(stored.whitelist, state.whitelist, sizeof(stored.whitelist));
    stored.whitelistCount = static_cast<uint8_t>(state.whitelistCount);

    Preferences prefs;
    if (!prefs.begin(SETTINGS_NAMESPACE, false)) {
        Serial.println("[SET] NVS open failed");
        return false;
    }

    bool ok = prefs.putBytes(SETTINGS_KEY, &stored, sizeof(stored)) == sizeof(stored);
    prefs.end();
    return ok;
}

}  // namespace

bool settingsLoad() {
    resetState();

    Preferences prefs;
    if (!prefs.begin(SETTINGS_NAMESPACE, true)) {
        Serial.println("[SET] NVS read failed");
        loaded = true;
        return false;
    }

    size_t len = prefs.getBytesLength(SETTINGS_KEY);
    if (len == sizeof(StoredSettings)) {
        StoredSettings stored{};
        size_t read = prefs.getBytes(SETTINGS_KEY, &stored, sizeof(stored));
        if (read == sizeof(stored) && stored.version == SETTINGS_VERSION) {
            memcpy(state.wifiSsid, stored.wifiSsid, sizeof(state.wifiSsid));
            memcpy(state.wifiPassword, stored.wifiPassword, sizeof(state.wifiPassword));
            memcpy(state.whitelist, stored.whitelist, sizeof(state.whitelist));
            state.whitelistCount = stored.whitelistCount;
        }
    }

    prefs.end();
    loaded = true;
    return true;
}

const GatewaySettings &settingsGet() {
    if (!loaded) {
        settingsLoad();
    }
    return state;
}

bool settingsSaveWiFi(const char *ssid, const char *password) {
    if (ssid == nullptr || password == nullptr) return false;
    strncpy(state.wifiSsid, ssid, sizeof(state.wifiSsid) - 1);
    state.wifiSsid[sizeof(state.wifiSsid) - 1] = '\0';
    strncpy(state.wifiPassword, password, sizeof(state.wifiPassword) - 1);
    state.wifiPassword[sizeof(state.wifiPassword) - 1] = '\0';
    return saveState();
}

bool settingsSaveWhitelist(const uint32_t *devices, size_t count) {
    if (devices == nullptr && count > 0) return false;
    if (count > GATEWAY_MAX_WHITELIST) count = GATEWAY_MAX_WHITELIST;

    memset(state.whitelist, 0, sizeof(state.whitelist));
    for (size_t i = 0; i < count; ++i) {
        state.whitelist[i] = devices[i];
    }
    state.whitelistCount = count;
    return saveState();
}

bool settingsParseWhitelistText(const char *text, uint32_t *devices,
                                size_t maxDevices, size_t &count) {
    count = 0;
    if (devices == nullptr || maxDevices == 0) return false;

    if (text == nullptr || text[0] == '\0') return true;

    char buffer[512];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    for (char *token = strtok(buffer, ", \r\n\t"); token != nullptr;
         token = strtok(nullptr, ", \r\n\t")) {
        if (count >= maxDevices) {
            return false;
        }

        char *end = nullptr;
        unsigned long value = strtoul(token, &end, 0);
        if (end == token || *end != '\0' || value > 0xFFFFFFFFUL) {
            return false;
        }

        devices[count++] = static_cast<uint32_t>(value);
    }

    return true;
}

void settingsFormatWhitelist(char *buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) return;

    const GatewaySettings &cfg = settingsGet();
    if (cfg.whitelistCount == 0) {
        strncpy(buffer, "accept all nodes", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }

    size_t used = 0;
    buffer[0] = '\0';
    for (size_t i = 0; i < cfg.whitelistCount; ++i) {
        int written = snprintf(buffer + used, bufferSize - used,
                               i == 0 ? "0x%08X" : ", 0x%08X",
                               cfg.whitelist[i]);
        if (written < 0 || static_cast<size_t>(written) >= bufferSize - used) {
            buffer[bufferSize - 1] = '\0';
            return;
        }
        used += static_cast<size_t>(written);
    }
}
