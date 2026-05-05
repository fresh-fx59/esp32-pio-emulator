#include <Wire.h>
#include <esp32sim/i2c.h>
#include <esp32sim/strict.h>

#include <cstdio>
#include <cstring>

TwoWire Wire(0);
TwoWire Wire1(1);

void TwoWire::beginTransmission(uint8_t addr) {
    auto& strict = esp32sim::Strict::instance();
    if (strict.enabled()) {
        if (addr > 0x7F) {
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                "Wire.beginTransmission(0x%02X) — I2C addresses must fit in 7 bits "
                "(0x00..0x7F)",
                (int)addr);
            strict.violation("ESP_SIM_E021", buf);
        }
        if (in_transmission_) {
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                "Wire.beginTransmission(0x%02X) called while previous transmission "
                "to 0x%02X was open — call endTransmission() first",
                (int)addr, (int)tx_addr_);
            strict.violation("ESP_SIM_E022", buf);
        }
    }
    in_transmission_ = true;
    tx_addr_ = addr;
    tx_len_ = 0;
}

size_t TwoWire::write(uint8_t b) {
    if (esp32sim::Strict::instance().enabled() && !in_transmission_) {
        esp32sim::Strict::instance().violation(
            "ESP_SIM_E020",
            "Wire.write called outside a beginTransmission/endTransmission pair");
    }
    if (tx_len_ < sizeof(tx_buf_)) {
        tx_buf_[tx_len_++] = b;
        return 1;
    }
    return 0;
}

size_t TwoWire::write(const uint8_t* data, size_t n) {
    if (esp32sim::Strict::instance().enabled() && !in_transmission_) {
        esp32sim::Strict::instance().violation(
            "ESP_SIM_E020",
            "Wire.write called outside a beginTransmission/endTransmission pair");
    }
    size_t written = 0;
    while (n-- && tx_len_ < sizeof(tx_buf_)) {
        tx_buf_[tx_len_++] = *data++;
        written++;
    }
    return written;
}

uint8_t TwoWire::endTransmission(bool /*stop*/) {
    auto result = esp32sim::I2CBus::for_index(bus_num_).write(tx_addr_, tx_buf_, tx_len_);
    in_transmission_ = false;
    tx_len_ = 0;
    switch (result) {
        case esp32sim::I2CResult::OK:        return 0;
        case esp32sim::I2CResult::NACK_ADDR: return 2;
        case esp32sim::I2CResult::NACK_DATA: return 3;
        default:                              return 4;
    }
}

size_t TwoWire::requestFrom(uint8_t addr, size_t n, bool /*stop*/) {
    if (n > sizeof(rx_buf_)) n = sizeof(rx_buf_);
    auto result = esp32sim::I2CBus::for_index(bus_num_).read(addr, rx_buf_, n);
    if (result != esp32sim::I2CResult::OK) {
        rx_len_ = 0;
        rx_pos_ = 0;
        return 0;
    }
    rx_len_ = n;
    rx_pos_ = 0;
    return n;
}

int TwoWire::available() {
    return (int)(rx_len_ - rx_pos_);
}

int TwoWire::read() {
    if (rx_pos_ >= rx_len_) return -1;
    return rx_buf_[rx_pos_++];
}

int TwoWire::peek() {
    if (rx_pos_ >= rx_len_) return -1;
    return rx_buf_[rx_pos_];
}
