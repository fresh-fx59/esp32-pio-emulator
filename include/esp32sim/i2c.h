// include/esp32sim/i2c.h — I2C bus simulator core
#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

namespace esp32sim {

enum class I2CResult : uint8_t {
    OK = 0,
    NACK_ADDR = 1,    // no device at that address
    NACK_DATA = 2,    // device NACK'd a data byte
    SHORT_READ = 3,   // device returned fewer bytes than requested
};

// Devices implementing this interface attach to an I2CBus and respond to
// I2C transactions. on_write/on_read are called by the bus when a transaction
// targets this device's address.
class I2CDevice {
public:
    virtual ~I2CDevice() = default;
    // The bus passed to a device on every transaction so it can chain access
    // (rare). For T2 the device just sees the data.
    virtual I2CResult on_write(const uint8_t* data, size_t len) = 0;
    virtual I2CResult on_read(uint8_t* buf, size_t len) = 0;
};

class I2CBus {
public:
    static constexpr int BUS_COUNT = 2;  // ESP32-S3 has Wire + Wire1
    static I2CBus& for_index(int n);
    static void reset_all();

    void attach(uint8_t addr, std::shared_ptr<I2CDevice> dev);
    void detach(uint8_t addr);
    bool has(uint8_t addr) const;

    // Master writes len bytes to addr. Returns OK on success, NACK_ADDR if
    // no device is attached, NACK_DATA if the device's on_write returned an
    // error.
    I2CResult write(uint8_t addr, const uint8_t* data, size_t len);

    // Master reads len bytes from addr.
    I2CResult read(uint8_t addr, uint8_t* buf, size_t len);

    // Combined write-then-read in one transaction (without STOP between).
    // Common pattern: write a register address, then read N bytes from that
    // register.
    I2CResult write_read(uint8_t addr, const uint8_t* w, size_t wn,
                         uint8_t* r, size_t rn);

    I2CBus() = default;

private:
    void reset_();
    struct Slot { uint8_t addr; std::shared_ptr<I2CDevice> dev; };
    std::vector<Slot> slots_;
};

}  // namespace esp32sim
