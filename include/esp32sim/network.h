// include/esp32sim/network.h — network state + event recording (T3)
#pragma once

#include <esp32sim/event_log.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace esp32sim {

enum class WifiState : uint8_t {
    IDLE = 0,
    CONNECTING = 1,
    CONNECTED = 2,
    DISCONNECTED = 3,
    CONNECT_FAILED = 4,
};

struct HttpResponse {
    int code = 200;
    std::string body;
    std::map<std::string, std::string> headers;
};

struct MqttMessage {
    std::string topic;
    std::string payload;
    bool retained = false;
    uint8_t qos = 0;
};

class Network {
public:
    static Network& instance();

    void reset();

    // WiFi
    WifiState wifi_state() const { return wifi_state_; }
    void set_wifi_state(WifiState s) { wifi_state_ = s; }
    int rssi() const { return rssi_; }
    void set_rssi(int v) { rssi_ = v; }
    std::string ssid() const { return ssid_; }
    std::string local_ip() const { return local_ip_; }
    void set_credentials(const std::string& ssid, const std::string& /*pass*/) {
        ssid_ = ssid;
        local_ip_ = "192.168.1.42";  // canned, sufficient for application-layer tests
        wifi_state_ = WifiState::CONNECTED;
    }
    void disconnect_wifi() {
        wifi_state_ = WifiState::DISCONNECTED;
    }

    // HTTP — pre-seeded responses keyed by exact URL
    void seed_http_response(const std::string& url, HttpResponse response);
    HttpResponse take_http_response(const std::string& url);

    // MQTT — pending deliveries to subscribed topics
    void mqtt_deliver(const std::string& topic, const std::string& payload);
    bool mqtt_pop_pending(MqttMessage& out);
    void mqtt_record_publish(const std::string& topic, const std::string& payload);
    void mqtt_record_subscribe(const std::string& topic);
    const std::vector<MqttMessage>& mqtt_publishes() const { return publishes_; }
    const std::vector<std::string>&  mqtt_subscribes() const { return subscribes_; }

private:
    Network() = default;

    WifiState wifi_state_ = WifiState::IDLE;
    int rssi_ = -50;
    std::string ssid_;
    std::string local_ip_ = "0.0.0.0";

    std::map<std::string, HttpResponse> http_responses_;

    std::vector<MqttMessage> mqtt_pending_;
    std::vector<MqttMessage> publishes_;
    std::vector<std::string> subscribes_;
};

}  // namespace esp32sim
