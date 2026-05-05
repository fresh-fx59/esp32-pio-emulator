// include/PubSubClient.h — fake of Nick O'Leary's PubSubClient MQTT lib (T3)
//
// Minimum viable surface: connect, publish, subscribe, loop, callback.
// Tests assert via Sim::mqtt().publishes() and Sim::mqtt().subscribes(),
// or drive incoming messages via Sim::mqtt().deliver(topic, payload).
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>

#ifdef __cplusplus

class PubSubClient {
public:
    typedef void (*Callback)(char* topic, uint8_t* payload, unsigned int length);

    PubSubClient() = default;
    explicit PubSubClient(Callback cb) : callback_(cb) {}

    PubSubClient& setServer(const char* host, uint16_t port) {
        host_ = host ? host : "";
        port_ = port;
        return *this;
    }
    PubSubClient& setCallback(Callback cb) {
        callback_ = cb;
        return *this;
    }
    PubSubClient& setKeepAlive(uint16_t /*sec*/) { return *this; }
    PubSubClient& setBufferSize(uint16_t /*sz*/) { return *this; }

    bool connect(const char* client_id);
    void disconnect();
    bool connected() const { return connected_; }

    bool publish(const char* topic, const char* payload);
    bool publish(const char* topic, const uint8_t* payload, unsigned int len);
    bool subscribe(const char* topic);
    bool unsubscribe(const char* topic);

    bool loop();  // delivers any pending mqtt_deliver()-d messages via callback

    int state() const { return connected_ ? 0 : -1; }

private:
    std::string host_;
    uint16_t port_ = 1883;
    bool connected_ = false;
    Callback callback_ = nullptr;
};

#endif  // __cplusplus
