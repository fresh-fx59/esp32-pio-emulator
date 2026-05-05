#include <esp32sim/event_log.h>
#include <esp32sim/i2c.h>

namespace esp32sim {

namespace {
I2CBus& bus(int n) {
    static I2CBus buses[I2CBus::BUS_COUNT];
    if (n < 0 || n >= I2CBus::BUS_COUNT) n = 0;
    return buses[n];
}
}  // namespace

I2CBus& I2CBus::for_index(int n) { return bus(n); }
void I2CBus::reset_all() {
    for (int i = 0; i < BUS_COUNT; ++i) bus(i).reset_();
}
void I2CBus::reset_() { slots_.clear(); }

void I2CBus::attach(uint8_t addr, std::shared_ptr<I2CDevice> dev) {
    detach(addr);
    slots_.push_back({addr, std::move(dev)});
}

void I2CBus::detach(uint8_t addr) {
    for (auto it = slots_.begin(); it != slots_.end(); ++it) {
        if (it->addr == addr) {
            slots_.erase(it);
            return;
        }
    }
}

bool I2CBus::has(uint8_t addr) const {
    for (const auto& s : slots_) {
        if (s.addr == addr) return true;
    }
    return false;
}

I2CResult I2CBus::write(uint8_t addr, const uint8_t* data, size_t len) {
    for (const auto& s : slots_) {
        if (s.addr == addr) {
            return s.dev->on_write(data, len);
        }
    }
    return I2CResult::NACK_ADDR;
}

I2CResult I2CBus::read(uint8_t addr, uint8_t* buf, size_t len) {
    for (const auto& s : slots_) {
        if (s.addr == addr) {
            return s.dev->on_read(buf, len);
        }
    }
    return I2CResult::NACK_ADDR;
}

I2CResult I2CBus::write_read(uint8_t addr, const uint8_t* w, size_t wn,
                              uint8_t* r, size_t rn) {
    auto wr = write(addr, w, wn);
    if (wr != I2CResult::OK) return wr;
    return read(addr, r, rn);
}

}  // namespace esp32sim
