#include <WiFi.h>
#include <esp32sim/network.h>
#include <esp32sim/strict.h>

#include <cstdio>

WiFiClass WiFi;

std::string IPAddress::toString() const {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u", a_, b_, c_, d_);
    return std::string(buf);
}

int WiFiClass::begin(const char* ssid, const char* password) {
    esp32sim::Network::instance().set_credentials(ssid ? ssid : "", password ? password : "");
    return WL_CONNECTED;
}

void WiFiClass::disconnect(bool /*wifi_off*/) {
    esp32sim::Network::instance().disconnect_wifi();
}

uint8_t WiFiClass::status() {
    using esp32sim::WifiState;
    switch (esp32sim::Network::instance().wifi_state()) {
        case WifiState::IDLE:           return WL_IDLE_STATUS;
        case WifiState::CONNECTING:     return WL_DISCONNECTED;  // pre-connected
        case WifiState::CONNECTED:      return WL_CONNECTED;
        case WifiState::DISCONNECTED:   return WL_DISCONNECTED;
        case WifiState::CONNECT_FAILED: return WL_CONNECT_FAILED;
    }
    return WL_IDLE_STATUS;
}

int WiFiClass::RSSI() {
    auto& net = esp32sim::Network::instance();
    if (esp32sim::Strict::instance().enabled() && net.wifi_state() != esp32sim::WifiState::CONNECTED) {
        esp32sim::Strict::instance().violation(
            "ESP_SIM_E050",
            "WiFi.RSSI() called before WiFi is connected — value is meaningless "
            "until WiFi.begin() has succeeded");
    }
    return net.rssi();
}

IPAddress WiFiClass::localIP() {
    auto& net = esp32sim::Network::instance();
    if (esp32sim::Strict::instance().enabled() && net.wifi_state() != esp32sim::WifiState::CONNECTED) {
        esp32sim::Strict::instance().violation(
            "ESP_SIM_E050",
            "WiFi.localIP() called before WiFi is connected — returns 0.0.0.0 "
            "until WiFi.begin() has succeeded");
    }
    auto ip = net.local_ip();
    int a = 0, b = 0, c = 0, d = 0;
    std::sscanf(ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d);
    return IPAddress((uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d);
}

const char* WiFiClass::SSID() {
    static thread_local std::string s;
    s = esp32sim::Network::instance().ssid();
    return s.c_str();
}
