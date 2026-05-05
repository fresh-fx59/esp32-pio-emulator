#include <HardwareSerial.h>
#include <esp32sim/strict.h>
#include <esp32sim/uart.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

HardwareSerial Serial(0);
HardwareSerial Serial1(1);
HardwareSerial Serial2(2);

void HardwareSerial::begin(unsigned long /*baud*/) {
    began_ = true;
}

namespace {
void check_begin(const HardwareSerial* s, const char* api, int uart_num) {
    if (esp32sim::Strict::instance().enabled() && !s->began()) {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "%s on Serial%d called before Serial%d.begin() — "
            "output will be silently discarded on real hardware",
            api, uart_num, uart_num);
        esp32sim::Strict::instance().violation("ESP_SIM_E010", buf, esp32sim::Severity::WARNING);
    }
}
}  // namespace

void HardwareSerial::emit_(const char* s) {
    check_begin(this, "Serial.print/write", uart_num_);
    auto& u = esp32sim::UartChannel::for_index(uart_num_);
    while (*s) u.tx_write_byte(static_cast<uint8_t>(*s++));
}

size_t HardwareSerial::print(const char* s) {
    if (!s) return 0;
    size_t n = std::strlen(s);
    emit_(s);
    return n;
}

size_t HardwareSerial::print(char c) {
    write(static_cast<uint8_t>(c));
    return 1;
}

size_t HardwareSerial::print(int n, int base) {
    return print(static_cast<long>(n), base);
}

size_t HardwareSerial::print(unsigned int n, int base) {
    return print(static_cast<unsigned long>(n), base);
}

size_t HardwareSerial::print(long n, int base) {
    char buf[34];
    if (base == 10) std::snprintf(buf, sizeof(buf), "%ld", n);
    else if (base == 16) std::snprintf(buf, sizeof(buf), "%lx", n);
    else if (base == 8) std::snprintf(buf, sizeof(buf), "%lo", n);
    else if (base == 2) {
        // simple binary formatter
        unsigned long u = (n < 0) ? (unsigned long)(-n) : (unsigned long)n;
        char tmp[66];
        int idx = 0;
        if (u == 0) tmp[idx++] = '0';
        while (u) { tmp[idx++] = '0' + (u & 1); u >>= 1; }
        if (n < 0) tmp[idx++] = '-';
        // reverse into buf
        for (int i = 0; i < idx; ++i) buf[i] = tmp[idx - 1 - i];
        buf[idx] = '\0';
    } else {
        std::snprintf(buf, sizeof(buf), "%ld", n);
    }
    return print(buf);
}

size_t HardwareSerial::print(unsigned long n, int base) {
    char buf[34];
    if (base == 16) std::snprintf(buf, sizeof(buf), "%lx", n);
    else if (base == 8) std::snprintf(buf, sizeof(buf), "%lo", n);
    else std::snprintf(buf, sizeof(buf), "%lu", n);
    return print(buf);
}

size_t HardwareSerial::print(double d, int decimals) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, d);
    return print(buf);
}

size_t HardwareSerial::println(const char* s) {
    size_t n = print(s);
    emit_("\r\n");
    return n + 2;
}

size_t HardwareSerial::println(int n, int base) {
    size_t r = print(n, base);
    emit_("\r\n");
    return r + 2;
}

size_t HardwareSerial::println(void) {
    emit_("\r\n");
    return 2;
}

size_t HardwareSerial::printf(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return 0;
    if ((size_t)n >= sizeof(buf)) {
        // truncated — emit what we have
        emit_(buf);
        return sizeof(buf) - 1;
    }
    emit_(buf);
    return (size_t)n;
}

size_t HardwareSerial::write(uint8_t b) {
    check_begin(this, "Serial.write", uart_num_);
    esp32sim::UartChannel::for_index(uart_num_).tx_write_byte(b);
    return 1;
}

size_t HardwareSerial::write(const uint8_t* buf, size_t n) {
    auto& u = esp32sim::UartChannel::for_index(uart_num_);
    for (size_t i = 0; i < n; ++i) u.tx_write_byte(buf[i]);
    return n;
}

size_t HardwareSerial::write(const char* s) {
    if (!s) return 0;
    size_t n = std::strlen(s);
    emit_(s);
    return n;
}

int HardwareSerial::available(void) {
    return (int)esp32sim::UartChannel::for_index(uart_num_).rx_size();
}

int HardwareSerial::read(void) {
    check_begin(this, "Serial.read", uart_num_);
    return esp32sim::UartChannel::for_index(uart_num_).rx_read_byte();
}

int HardwareSerial::peek(void) {
    return esp32sim::UartChannel::for_index(uart_num_).rx_peek();
}
