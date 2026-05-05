#include <PubSubClient.h>
#include <esp32sim/network.h>
#include <esp32sim/strict.h>

#include <cstring>

bool PubSubClient::connect(const char* /*client_id*/) {
    connected_ = true;
    return true;
}

void PubSubClient::disconnect() { connected_ = false; }

bool PubSubClient::publish(const char* topic, const char* payload) {
    if (!connected_) {
        if (esp32sim::Strict::instance().enabled()) {
            esp32sim::Strict::instance().violation(
                "ESP_SIM_E052",
                std::string("PubSubClient::publish on disconnected client (topic=") +
                (topic ? topic : "") + ") — message will be silently dropped");
        }
        return false;
    }
    esp32sim::Network::instance().mqtt_record_publish(
        topic ? topic : "", payload ? payload : "");
    return true;
}

bool PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int len) {
    if (!connected_) return false;
    esp32sim::Network::instance().mqtt_record_publish(
        topic ? topic : "",
        std::string(reinterpret_cast<const char*>(payload), len));
    return true;
}

bool PubSubClient::subscribe(const char* topic) {
    if (!connected_) return false;
    esp32sim::Network::instance().mqtt_record_subscribe(topic ? topic : "");
    return true;
}

bool PubSubClient::unsubscribe(const char* /*topic*/) {
    return connected_;
}

bool PubSubClient::loop() {
    if (!connected_) return false;
    esp32sim::MqttMessage msg;
    while (esp32sim::Network::instance().mqtt_pop_pending(msg)) {
        if (callback_) {
            // PubSubClient's callback signature uses non-const pointers.
            char topic_buf[256];
            std::strncpy(topic_buf, msg.topic.c_str(), sizeof(topic_buf) - 1);
            topic_buf[sizeof(topic_buf) - 1] = '\0';
            callback_(topic_buf,
                      reinterpret_cast<uint8_t*>(const_cast<char*>(msg.payload.data())),
                      (unsigned int)msg.payload.size());
        }
    }
    return true;
}
