#include <esp32sim/i2c.h>
#include <unity.h>

#include <memory>

using namespace esp32sim;

namespace {

// A minimal echo device: stores last write, returns canned bytes on read.
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

void test_bus_starts_empty(void) {
    TEST_ASSERT_FALSE(I2CBus::for_index(0).has(0x68));
}

void test_attach_and_has(void) {
    auto dev = std::make_shared<EchoDevice>();
    I2CBus::for_index(0).attach(0x68, dev);
    TEST_ASSERT_TRUE(I2CBus::for_index(0).has(0x68));
    TEST_ASSERT_FALSE(I2CBus::for_index(0).has(0x77));
}

void test_write_to_unattached_returns_nack_addr(void) {
    uint8_t data[1] = {0xAA};
    TEST_ASSERT_EQUAL_INT((int)I2CResult::NACK_ADDR,
                          (int)I2CBus::for_index(0).write(0x99, data, 1));
}

void test_write_routes_to_attached_device(void) {
    auto dev = std::make_shared<EchoDevice>();
    I2CBus::for_index(0).attach(0x68, dev);
    uint8_t data[3] = {0x01, 0x02, 0x03};
    TEST_ASSERT_EQUAL_INT((int)I2CResult::OK,
                          (int)I2CBus::for_index(0).write(0x68, data, 3));
    TEST_ASSERT_EQUAL_size_t(3, dev->last_write.size());
    TEST_ASSERT_EQUAL_UINT8(0x01, dev->last_write[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, dev->last_write[1]);
    TEST_ASSERT_EQUAL_UINT8(0x03, dev->last_write[2]);
}

void test_read_routes_to_attached_device(void) {
    auto dev = std::make_shared<EchoDevice>();
    dev->read_response = {0xCA, 0xFE};
    I2CBus::for_index(0).attach(0x68, dev);
    uint8_t buf[2] = {0, 0};
    TEST_ASSERT_EQUAL_INT((int)I2CResult::OK,
                          (int)I2CBus::for_index(0).read(0x68, buf, 2));
    TEST_ASSERT_EQUAL_UINT8(0xCA, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFE, buf[1]);
}

void test_write_read_combined(void) {
    auto dev = std::make_shared<EchoDevice>();
    dev->read_response = {0x42};
    I2CBus::for_index(0).attach(0x68, dev);
    uint8_t reg[1] = {0x05};  // "select register 5"
    uint8_t out[1] = {0};
    TEST_ASSERT_EQUAL_INT((int)I2CResult::OK,
                          (int)I2CBus::for_index(0).write_read(0x68, reg, 1, out, 1));
    TEST_ASSERT_EQUAL_size_t(1, dev->last_write.size());
    TEST_ASSERT_EQUAL_UINT8(0x05, dev->last_write[0]);
    TEST_ASSERT_EQUAL_UINT8(0x42, out[0]);
}

void test_two_buses_isolated(void) {
    auto dev0 = std::make_shared<EchoDevice>();
    auto dev1 = std::make_shared<EchoDevice>();
    I2CBus::for_index(0).attach(0x68, dev0);
    I2CBus::for_index(1).attach(0x68, dev1);
    uint8_t data[1] = {0xAA};
    I2CBus::for_index(0).write(0x68, data, 1);
    TEST_ASSERT_EQUAL_size_t(1, dev0->last_write.size());
    TEST_ASSERT_EQUAL_size_t(0, dev1->last_write.size());
}

void test_detach_removes_device(void) {
    auto dev = std::make_shared<EchoDevice>();
    I2CBus::for_index(0).attach(0x68, dev);
    I2CBus::for_index(0).detach(0x68);
    TEST_ASSERT_FALSE(I2CBus::for_index(0).has(0x68));
    uint8_t data[1] = {0xAA};
    TEST_ASSERT_EQUAL_INT((int)I2CResult::NACK_ADDR,
                          (int)I2CBus::for_index(0).write(0x68, data, 1));
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_bus_starts_empty);
    RUN_TEST(test_attach_and_has);
    RUN_TEST(test_write_to_unattached_returns_nack_addr);
    RUN_TEST(test_write_routes_to_attached_device);
    RUN_TEST(test_read_routes_to_attached_device);
    RUN_TEST(test_write_read_combined);
    RUN_TEST(test_two_buses_isolated);
    RUN_TEST(test_detach_removes_device);
    return UNITY_END();
}
