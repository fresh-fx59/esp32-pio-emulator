#include <SPI.h>
#include <esp32sim/spi.h>
#include <unity.h>

#include <memory>

using namespace esp32sim;

namespace {

// EchoBack device: returns whatever was sent, plus 1.
class EchoPlusOne : public SpiDevice {
public:
    uint8_t on_transfer(uint8_t mosi) override { return (uint8_t)(mosi + 1); }
};

// Fixed-response device: returns from a buffer.
class FixedResponse : public SpiDevice {
public:
    std::vector<uint8_t> response;
    size_t pos = 0;
    uint8_t on_transfer(uint8_t /*mosi*/) override {
        if (pos < response.size()) return response[pos++];
        return 0xFF;
    }
};

}  // namespace

void setUp(void) { SpiBus::reset_all(); }
void tearDown(void) {}

void test_spi_no_transaction_returns_ff(void) {
    TEST_ASSERT_EQUAL_UINT8(0xFF, SpiBus::for_index(0).transfer(0x42));
}

void test_spi_attach_transfer_via_bus(void) {
    auto dev = std::make_shared<EchoPlusOne>();
    SpiBus::for_index(0).attach(/*cs=*/5, dev);
    SpiBus::for_index(0).begin_transaction(5);
    TEST_ASSERT_EQUAL_UINT8(0x43, SpiBus::for_index(0).transfer(0x42));
    SpiBus::for_index(0).end_transaction();
}

void test_spi_via_SPIClass(void) {
    auto dev = std::make_shared<EchoPlusOne>();
    SpiBus::for_index(0).attach(5, dev);

    SPI.setCS(5);
    uint8_t resp = SPI.transfer(0x10);
    SPI.endTransaction();
    TEST_ASSERT_EQUAL_UINT8(0x11, resp);
}

void test_spi_transfer16(void) {
    auto dev = std::make_shared<FixedResponse>();
    dev->response = {0xAA, 0xBB};
    SpiBus::for_index(0).attach(5, dev);

    SPI.setCS(5);
    uint16_t r = SPI.transfer16(0x0000);
    SPI.endTransaction();
    TEST_ASSERT_EQUAL_UINT16(0xAABB, r);
}

void test_spi_transfer_buffer(void) {
    auto dev = std::make_shared<EchoPlusOne>();
    SpiBus::for_index(0).attach(5, dev);

    uint8_t buf[3] = {0x10, 0x20, 0x30};
    SPI.setCS(5);
    SPI.transfer(buf, 3);
    SPI.endTransaction();
    TEST_ASSERT_EQUAL_UINT8(0x11, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x21, buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x31, buf[2]);
}

void test_spi_two_devices_routed_by_cs(void) {
    auto dev_a = std::make_shared<EchoPlusOne>();
    auto dev_b = std::make_shared<FixedResponse>();
    dev_b->response = {0xCC};
    SpiBus::for_index(0).attach(5, dev_a);
    SpiBus::for_index(0).attach(6, dev_b);

    SPI.setCS(5);
    TEST_ASSERT_EQUAL_UINT8(0x01, SPI.transfer(0x00));
    SPI.endTransaction();

    SPI.setCS(6);
    TEST_ASSERT_EQUAL_UINT8(0xCC, SPI.transfer(0x00));
    SPI.endTransaction();
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_spi_no_transaction_returns_ff);
    RUN_TEST(test_spi_attach_transfer_via_bus);
    RUN_TEST(test_spi_via_SPIClass);
    RUN_TEST(test_spi_transfer16);
    RUN_TEST(test_spi_transfer_buffer);
    RUN_TEST(test_spi_two_devices_routed_by_cs);
    return UNITY_END();
}
