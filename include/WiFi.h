// include/WiFi.h — fake arduino-esp32 WiFi (T3)
#pragma once

#include <stdint.h>
#include <string>

#ifdef __cplusplus

// Compatible with arduino-esp32's wl_status_t enum
enum WiFiStatus : uint8_t {
    WL_NO_SHIELD = 255,
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL = 1,
    WL_SCAN_COMPLETED = 2,
    WL_CONNECTED = 3,
    WL_CONNECT_FAILED = 4,
    WL_CONNECTION_LOST = 5,
    WL_DISCONNECTED = 6,
};

// Minimal IPAddress for compatibility
class IPAddress {
public:
    IPAddress() : a_(0), b_(0), c_(0), d_(0) {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : a_(a), b_(b), c_(c), d_(d) {}
    operator uint32_t() const {
        return ((uint32_t)a_ << 24) | ((uint32_t)b_ << 16) | ((uint32_t)c_ << 8) | d_;
    }
    std::string toString() const;
private:
    uint8_t a_, b_, c_, d_;
};

class WiFiClass {
public:
    int begin(const char* ssid, const char* password = "");
    void disconnect(bool wifi_off = false);
    uint8_t status();
    int RSSI();
    IPAddress localIP();
    const char* SSID();
    void mode(int /*m*/) {}  // STA/AP modes accepted but no-op
};

extern WiFiClass WiFi;

#endif  // __cplusplus
