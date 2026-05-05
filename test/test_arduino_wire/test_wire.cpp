#include <Arduino.h>
#include <Wire.h>
#include <esp32sim/i2c.h>
#include <unity.h>

#include <memory>

using namespace esp32sim;

namespace {

// Same EchoDevice as test_core_i2c (duplicated here because Unity tests are
// independent TUs).
class EchoDevice : public I2CDevice {
public:
    std::vector<uint8_t> last_write;
    std::vector<uint8_t> read_response;

    I2CResult on_write(const uint8_t* data, size_t len) override {
        last_write.assign(data, data + len);
        return I2CResult::OK;
    }
    I2CResult on_read(uint8_t* buf, size_t len) override {
        for (size_t i = 0; i < len; ++i) {
            buf[i] = (i < read_response.size()) ? read_response[i] : 0xFF;
        }
        return I2CResult::OK;
    }
};

}  // namespace

void setUp(void) { I2CBus::reset_all(); }
void tearDown(void) {}

void test_wire_begintransmission_write_endtransmission(void) {
    auto dev = std::make_shared<EchoDevice>();
    I2CBus::for_index(0).attach(0x68, dev);

    Wire.beginTransmission(0x68);
    Wire.write(0x01);
    Wire.write(0xAA);
    uint8_t result = Wire.endTransmission();

    TEST_ASSERT_EQUAL_UINT8(0, result);  // 0 = success
    TEST_ASSERT_EQUAL_size_t(2, dev->last_write.size());
    TEST_ASSERT_EQUAL_UINT8(0x01, dev->last_write[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, dev->last_write[1]);
}

void test_wire_endtransmission_returns_2_on_nack_addr(void) {
    Wire.beginTransmission(0x99);  // no device
    Wire.write(0x01);
    TEST_ASSERT_EQUAL_UINT8(2, Wire.endTransmission());  // 2 = NACK on addr
}

void test_wire_requestfrom_then_read(void) {
    auto dev = std::make_shared<EchoDevice>();
    dev->read_response = {0xDE, 0xAD, 0xBE, 0xEF};
    I2CBus::for_index(0).attach(0x68, dev);

    size_t got = Wire.requestFrom((uint8_t)0x68, (size_t)4);
    TEST_ASSERT_EQUAL_size_t(4, got);
    TEST_ASSERT_EQUAL_INT(4, Wire.available());

    TEST_ASSERT_EQUAL_INT(0xDE, Wire.read());
    TEST_ASSERT_EQUAL_INT(0xAD, Wire.read());
    TEST_ASSERT_EQUAL_INT(0xBE, Wire.read());
    TEST_ASSERT_EQUAL_INT(0xEF, Wire.read());
    TEST_ASSERT_EQUAL_INT(-1, Wire.read());
}

void test_wire_peek_does_not_consume(void) {
    auto dev = std::make_shared<EchoDevice>();
    dev->read_response = {0x12, 0x34};
    I2CBus::for_index(0).attach(0x68, dev);
    Wire.requestFrom((uint8_t)0x68, (size_t)2);
    TEST_ASSERT_EQUAL_INT(0x12, Wire.peek());
    TEST_ASSERT_EQUAL_INT(0x12, Wire.peek());
    TEST_ASSERT_EQUAL_INT(2, Wire.available());
}

void test_wire1_uses_bus_1(void) {
    auto dev = std::make_shared<EchoDevice>();
    I2CBus::for_index(1).attach(0x42, dev);

    Wire1.beginTransmission(0x42);
    Wire1.write(0x99);
    Wire1.endTransmission();

    TEST_ASSERT_EQUAL_size_t(1, dev->last_write.size());
    TEST_ASSERT_EQUAL_UINT8(0x99, dev->last_write[0]);
}

void test_wire_buffer_writes(void) {
    auto dev = std::make_shared<EchoDevice>();
    I2CBus::for_index(0).attach(0x68, dev);
    uint8_t buf[3] = {0xCA, 0xFE, 0xBE};
    Wire.beginTransmission(0x68);
    Wire.write(buf, 3);
    Wire.endTransmission();
    TEST_ASSERT_EQUAL_size_t(3, dev->last_write.size());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_wire_begintransmission_write_endtransmission);
    RUN_TEST(test_wire_endtransmission_returns_2_on_nack_addr);
    RUN_TEST(test_wire_requestfrom_then_read);
    RUN_TEST(test_wire_peek_does_not_consume);
    RUN_TEST(test_wire1_uses_bus_1);
    RUN_TEST(test_wire_buffer_writes);
    return UNITY_END();
}
