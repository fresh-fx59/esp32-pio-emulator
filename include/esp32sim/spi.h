// include/esp32sim/spi.h — SPI bus simulator
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace esp32sim {

class SpiDevice {
public:
    virtual ~SpiDevice() = default;
    // Called once per byte exchanged in a transaction. Device returns the
    // MISO byte for the master-driven MOSI byte.
    virtual uint8_t on_transfer(uint8_t mosi) = 0;
};

class SpiBus {
public:
    static constexpr int BUS_COUNT = 2;  // ESP32-S3 has HSPI + VSPI
    static SpiBus& for_index(int n);
    static void reset_all();

    void attach(int cs_pin, std::shared_ptr<SpiDevice> dev);
    void detach(int cs_pin);

    void begin_transaction(int cs_pin);
    uint8_t  transfer(uint8_t mosi);          // exchanges one byte
    uint16_t transfer16(uint16_t mosi);       // exchanges two bytes (MSB first)
    void     transfer_buffer(uint8_t* buf, size_t n);  // in-place
    void end_transaction();

    SpiBus() = default;

private:
    void reset_();
    struct Slot { int cs_pin; std::shared_ptr<SpiDevice> dev; };
    std::vector<Slot> slots_;
    int active_cs_ = -1;
};

}  // namespace esp32sim
