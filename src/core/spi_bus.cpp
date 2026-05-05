#include <esp32sim/spi.h>

namespace esp32sim {

namespace {
SpiBus& bus(int n) {
    static SpiBus buses[SpiBus::BUS_COUNT];
    if (n < 0 || n >= SpiBus::BUS_COUNT) n = 0;
    return buses[n];
}
}  // namespace

SpiBus& SpiBus::for_index(int n) { return bus(n); }
void SpiBus::reset_all() {
    for (int i = 0; i < BUS_COUNT; ++i) bus(i).reset_();
}
void SpiBus::reset_() { slots_.clear(); active_cs_ = -1; }

void SpiBus::attach(int cs_pin, std::shared_ptr<SpiDevice> dev) {
    detach(cs_pin);
    slots_.push_back({cs_pin, std::move(dev)});
}

void SpiBus::detach(int cs_pin) {
    for (auto it = slots_.begin(); it != slots_.end(); ++it) {
        if (it->cs_pin == cs_pin) {
            slots_.erase(it);
            return;
        }
    }
}

void SpiBus::begin_transaction(int cs_pin) { active_cs_ = cs_pin; }
void SpiBus::end_transaction() { active_cs_ = -1; }

uint8_t SpiBus::transfer(uint8_t mosi) {
    if (active_cs_ < 0) return 0xFF;  // no transaction active
    for (auto& s : slots_) {
        if (s.cs_pin == active_cs_) {
            return s.dev->on_transfer(mosi);
        }
    }
    return 0xFF;
}

uint16_t SpiBus::transfer16(uint16_t mosi) {
    uint8_t hi = transfer((uint8_t)((mosi >> 8) & 0xFF));
    uint8_t lo = transfer((uint8_t)(mosi & 0xFF));
    return ((uint16_t)hi << 8) | lo;
}

void SpiBus::transfer_buffer(uint8_t* buf, size_t n) {
    for (size_t i = 0; i < n; ++i) buf[i] = transfer(buf[i]);
}

}  // namespace esp32sim
