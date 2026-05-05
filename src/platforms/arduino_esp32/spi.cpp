#include <SPI.h>
#include <esp32sim/spi.h>

SPIClass SPI(0);

void SPIClass::setCS(int cs_pin) {
    cs_pin_ = cs_pin;
    esp32sim::SpiBus::for_index(bus_num_).begin_transaction(cs_pin);
}

void SPIClass::endTransaction() {
    esp32sim::SpiBus::for_index(bus_num_).end_transaction();
    cs_pin_ = -1;
}

uint8_t SPIClass::transfer(uint8_t mosi) {
    return esp32sim::SpiBus::for_index(bus_num_).transfer(mosi);
}

uint16_t SPIClass::transfer16(uint16_t mosi) {
    return esp32sim::SpiBus::for_index(bus_num_).transfer16(mosi);
}

void SPIClass::transfer(uint8_t* buf, size_t n) {
    esp32sim::SpiBus::for_index(bus_num_).transfer_buffer(buf, n);
}
