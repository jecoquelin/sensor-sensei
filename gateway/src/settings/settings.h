#pragma once

#include <Arduino.h>

constexpr size_t GATEWAY_MAX_WHITELIST = 16;
constexpr size_t WIFI_SSID_LEN = 33;
constexpr size_t WIFI_PASSWORD_LEN = 65;

struct GatewaySettings {
    char wifiSsid[WIFI_SSID_LEN];
    char wifiPassword[WIFI_PASSWORD_LEN];
    uint32_t whitelist[GATEWAY_MAX_WHITELIST];
    size_t whitelistCount;
};

bool settingsLoad();
const GatewaySettings &settingsGet();
bool settingsSaveWiFi(const char *ssid, const char *password);
bool settingsSaveWhitelist(const uint32_t *devices, size_t count);
bool settingsParseWhitelistText(const char *text, uint32_t *devices,
                                size_t maxDevices, size_t &count);
void settingsFormatWhitelist(char *buffer, size_t bufferSize);
