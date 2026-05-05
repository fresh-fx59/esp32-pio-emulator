// include/HTTPClient.h — fake arduino-esp32 HTTPClient (T3)
#pragma once

#include <stdint.h>
#include <map>
#include <string>

#ifdef __cplusplus

class HTTPClient {
public:
    HTTPClient() = default;

    void begin(const char* url) { url_ = url ? url : ""; }
    void begin(const std::string& url) { url_ = url; }
    void end() {}

    void addHeader(const std::string& name, const std::string& value) {
        headers_[name] = value;
    }
    void setTimeout(int /*ms*/) {}

    int GET();
    int POST(const std::string& body);
    int POST(const uint8_t* body, size_t len);

    std::string getString();
    int getSize() const { return (int)last_response_body_.size(); }
    std::string getStreamString() { return getString(); }
    std::string header(const std::string& name);

private:
    std::string url_;
    std::map<std::string, std::string> headers_;
    std::string last_response_body_;
    std::map<std::string, std::string> last_response_headers_;

    int execute_(const std::string& method, const std::string& body);
};

#endif  // __cplusplus
