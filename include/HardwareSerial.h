#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus

class HardwareSerial {
public:
    explicit HardwareSerial(int uart_num) : uart_num_(uart_num) {}

    // Real arduino-esp32 begin signature has many optional params; T1 keeps
    // it simple and ignores baud/config since the sim has no electrical
    // representation.
    void begin(unsigned long /*baud*/) {}
    void end(void) {}
    void flush(void) {}

    // Print API
    size_t print(const char* s);
    size_t print(int n, int base = 10);
    size_t print(unsigned int n, int base = 10);
    size_t print(long n, int base = 10);
    size_t print(unsigned long n, int base = 10);
    size_t print(double d, int decimals = 2);
    size_t print(char c);
    size_t println(const char* s);
    size_t println(int n, int base = 10);
    size_t println(void);
    size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

    // Write API
    size_t write(uint8_t b);
    size_t write(const uint8_t* buf, size_t n);
    size_t write(const char* s);

    // Read API
    int available(void);
    int read(void);
    int peek(void);

private:
    int uart_num_;
    void emit_(const char* s);
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;

#endif  // __cplusplus
