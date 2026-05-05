#include <HTTPClient.h>
#include <esp32sim/event_log.h>
#include <esp32sim/network.h>
#include <esp32sim/strict.h>

namespace {
// Stash the last request method/body so they're discoverable via the event
// log. We use UART_TX as a generic channel for now; T3.5 will introduce
// dedicated NETWORK_TXN event kinds with structured payload.
}

int HTTPClient::execute_(const std::string& method, const std::string& body) {
    if (esp32sim::Strict::instance().enabled() && url_.empty()) {
        esp32sim::Strict::instance().violation(
            "ESP_SIM_E051",
            std::string("HTTPClient::") + method + "() called without a prior "
            "HTTPClient::begin(url) — request URL is empty");
    }
    // Record the request as a ready-to-inspect side effect.
    esp32sim::Network::instance().mqtt_record_publish(
        std::string("HTTP:") + method + " " + url_, body);
    auto resp = esp32sim::Network::instance().take_http_response(url_);
    last_response_body_ = std::move(resp.body);
    last_response_headers_ = std::move(resp.headers);
    return resp.code;
}

int HTTPClient::GET()                                  { return execute_("GET", ""); }
int HTTPClient::POST(const std::string& body)          { return execute_("POST", body); }
int HTTPClient::POST(const uint8_t* body, size_t len) {
    return execute_("POST", std::string((const char*)body, len));
}

std::string HTTPClient::getString() { return last_response_body_; }

std::string HTTPClient::header(const std::string& name) {
    auto it = last_response_headers_.find(name);
    return it == last_response_headers_.end() ? "" : it->second;
}
