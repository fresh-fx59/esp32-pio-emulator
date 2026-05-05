#include <Wire.h>
#include <esp32sim/i2c.h>

#include <cstring>

TwoWire Wire(0);
TwoWire Wire1(1);

void TwoWire::beginTransmission(uint8_t addr) {
    tx_addr_ = addr;
    tx_len_ = 0;
}

size_t TwoWire::write(uint8_t b) {
    if (tx_len_ < sizeof(tx_buf_)) {
        tx_buf_[tx_len_++] = b;
        return 1;
    }
    return 0;
}

size_t TwoWire::write(const uint8_t* data, size_t n) {
    size_t written = 0;
    while (n-- && tx_len_ < sizeof(tx_buf_)) {
        tx_buf_[tx_len_++] = *data++;
        written++;
    }
    return written;
}

uint8_t TwoWire::endTransmission(bool /*stop*/) {
    auto result = esp32sim::I2CBus::for_index(bus_num_).write(tx_addr_, tx_buf_, tx_len_);
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
