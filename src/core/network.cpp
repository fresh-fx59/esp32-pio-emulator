#include <esp32sim/network.h>

namespace esp32sim {

Network& Network::instance() {
    static Network n;
    return n;
}

void Network::reset() {
    wifi_state_ = WifiState::IDLE;
    rssi_ = -50;
    ssid_.clear();
    local_ip_ = "0.0.0.0";
    http_responses_.clear();
    mqtt_pending_.clear();
    publishes_.clear();
    subscribes_.clear();
}

void Network::seed_http_response(const std::string& url, HttpResponse response) {
    http_responses_[url] = std::move(response);
}

HttpResponse Network::take_http_response(const std::string& url) {
    auto it = http_responses_.find(url);
    if (it == http_responses_.end()) {
        // No pre-seeded response — return 404 by default. Tests that care
        // should always seed a response.
        return HttpResponse{404, "", {}};
    }
    HttpResponse r = std::move(it->second);
    http_responses_.erase(it);
    return r;
}

void Network::mqtt_deliver(const std::string& topic, const std::string& payload) {
    MqttMessage m;
    m.topic = topic;
    m.payload = payload;
    mqtt_pending_.push_back(std::move(m));
}

bool Network::mqtt_pop_pending(MqttMessage& out) {
    if (mqtt_pending_.empty()) return false;
    out = mqtt_pending_.front();
    mqtt_pending_.erase(mqtt_pending_.begin());
    return true;
}

void Network::mqtt_record_publish(const std::string& topic, const std::string& payload) {
    MqttMessage m;
    m.topic = topic;
    m.payload = payload;
    publishes_.push_back(std::move(m));
}

void Network::mqtt_record_subscribe(const std::string& topic) {
    subscribes_.push_back(topic);
}

}  // namespace esp32sim
