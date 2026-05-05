#include <esp32sim/event_log.h>
#include <esp32sim/uart.h>

namespace esp32sim {

namespace {
constexpr int kUartCount = 3;
UartChannel& channel(int n) {
    static UartChannel chans[kUartCount];
    if (n < 0 || n >= kUartCount) n = 0;
    return chans[n];
}
}  // namespace

UartChannel& UartChannel::for_index(int n) { return channel(n); }

void UartChannel::reset_all() {
    for (int i = 0; i < kUartCount; ++i) channel(i).reset_();
}

void UartChannel::reset_() {
    tx_drainable_.clear();
    tx_full_history_.clear();
    rx_buffer_.clear();
}

void UartChannel::tx_write_byte(uint8_t b) {
    tx_drainable_ += static_cast<char>(b);
    tx_full_history_ += static_cast<char>(b);
    EventLog::instance().emit(Event{EventKind::UART_TX, /*pin=uart_index*/ 0, (int)b});
}

std::string UartChannel::drain_tx() {
    std::string out = tx_drainable_;
    tx_drainable_.clear();
    return out;
}

std::string UartChannel::tx_history() const { return tx_full_history_; }

void UartChannel::rx_inject(std::string_view s) {
    for (char c : s) rx_buffer_.push_back(static_cast<uint8_t>(c));
}

int UartChannel::rx_read_byte() {
    if (rx_buffer_.empty()) return -1;
    int b = rx_buffer_.front();
    rx_buffer_.pop_front();
    EventLog::instance().emit(Event{EventKind::UART_RX, 0, b});
    return b;
}

int UartChannel::rx_peek() const {
    if (rx_buffer_.empty()) return -1;
    return rx_buffer_.front();
}

}  // namespace esp32sim
