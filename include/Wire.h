// include/Wire.h — fake arduino-esp32 Wire / TwoWire
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
class TwoWire {
public:
    explicit TwoWire(int bus_num) : bus_num_(bus_num) {}

    void begin() {}
    void begin(int /*sda*/, int /*scl*/, uint32_t /*freq*/ = 100000) {}
    void setClock(uint32_t /*freq*/) {}
    void end() {}

    // Master write transaction
    void beginTransmission(uint8_t addr);
    size_t write(uint8_t b);
    size_t write(const uint8_t* data, size_t n);
    uint8_t endTransmission(bool stop = true);
    // Returns: 0 success, 1 data too long, 2 NACK on addr, 3 NACK on data, 4 other.

    // Master read transaction
    size_t requestFrom(uint8_t addr, size_t n, bool stop = true);

    int available();
    int read();
    int peek();
    void flush() {}

private:
    int bus_num_;
    uint8_t tx_addr_ = 0;
    uint8_t tx_buf_[64] = {};
    size_t tx_len_ = 0;
    uint8_t rx_buf_[64] = {};
    size_t rx_len_ = 0;
    size_t rx_pos_ = 0;
};

extern TwoWire Wire;
extern TwoWire Wire1;

#endif  // __cplusplus
