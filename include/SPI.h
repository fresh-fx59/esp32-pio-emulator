// include/SPI.h — fake arduino-esp32 SPI
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus

class SPISettings {
public:
    SPISettings() = default;
    SPISettings(uint32_t clock, uint8_t bit_order, uint8_t data_mode)
        : clock_(clock), bit_order_(bit_order), data_mode_(data_mode) {}

private:
    uint32_t clock_ = 1000000;
    uint8_t bit_order_ = 0;  // MSBFIRST
    uint8_t data_mode_ = 0;  // SPI_MODE0
};

class SPIClass {
public:
    explicit SPIClass(int bus_num = 0) : bus_num_(bus_num) {}

    void begin() {}
    void end() {}

    void beginTransaction(SPISettings /*settings*/) {
        // Default CS is -1 here; users must call setCS or use the
        // Arduino way of toggling CS pin manually.
        // For sim we treat begin/end as "active=cs_pin set via setCS".
    }
    void endTransaction();

    // Convenience: explicit CS-based transaction (matches the typical
    // ESP32 idiom of digitalWrite(cs, LOW) before transfer + HIGH after).
    void setCS(int cs_pin);

    uint8_t  transfer(uint8_t mosi);
    uint16_t transfer16(uint16_t mosi);
    void     transfer(uint8_t* buf, size_t n);

private:
    int bus_num_;
    int cs_pin_ = -1;
};

extern SPIClass SPI;

#endif  // __cplusplus
