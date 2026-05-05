#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>

namespace esp32sim {

class UartChannel {
public:
    // ESP32-S3 has UART0/1/2. for_index(N) returns the channel; out-of-range
    // returns channel 0 (mirrors arduino-esp32's silent fallback for
    // unsupported indices).
    static UartChannel& for_index(int n);
    static void reset_all();

    // TX side — sketch writes here, tests drain or query.
    void tx_write_byte(uint8_t b);
    std::string drain_tx();           // returns and clears the un-drained portion
    std::string tx_history() const;   // entire TX log since last reset_all()

    // RX side — tests inject, sketch reads.
    void rx_inject(std::string_view s);
    bool rx_available() const { return !rx_buffer_.empty(); }
    size_t rx_size() const { return rx_buffer_.size(); }
    int rx_read_byte();               // -1 if empty
    int rx_peek() const;              // -1 if empty

    size_t tx_buffer() const { return tx_drainable_.size(); }  // for tests

    // Constructor is public because we keep an array of these in storage.
    // Users should not construct directly; go through for_index().
    UartChannel() = default;

private:
    void reset_();

    std::string tx_drainable_;        // grows on write, cleared on drain_tx
    std::string tx_full_history_;     // grows on write, only cleared on reset_all
    std::deque<uint8_t> rx_buffer_;
};

}  // namespace esp32sim
