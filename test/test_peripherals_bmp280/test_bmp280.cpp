#include <Wire.h>
#include <esp32sim/i2c.h>
#include <peripherals/FakeBMP280.h>
#include <unity.h>

#include <memory>

using namespace esp32sim;
using esp32sim::peripherals::FakeBMP280;

void setUp(void) { I2CBus::reset_all(); }
void tearDown(void) {}

void test_chip_id(void) {
    auto bmp = std::make_shared<FakeBMP280>();
    TEST_ASSERT_EQUAL_UINT8(0x58, bmp->reg(0xD0));
}

void test_calibration_coefficients_present(void) {
    auto bmp = std::make_shared<FakeBMP280>();
    // dig_T1 = 27504 — little-endian: 0x70, 0x6B
    TEST_ASSERT_EQUAL_UINT8(0x70, bmp->reg(0x88));
    TEST_ASSERT_EQUAL_UINT8(0x6B, bmp->reg(0x89));
}

void test_via_I2C_read_chip_id(void) {
    auto bmp = std::make_shared<FakeBMP280>();
    I2CBus::for_index(0).attach(FakeBMP280::DEFAULT_ADDR, bmp);

    Wire.beginTransmission(0x76);
    Wire.write((uint8_t)0xD0);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x76, (size_t)1);
    TEST_ASSERT_EQUAL_INT(1, Wire.available());
    TEST_ASSERT_EQUAL_INT(0x58, Wire.read());
}

void test_temperature_registers_change_on_setTemperature(void) {
    auto bmp = std::make_shared<FakeBMP280>();
    uint8_t before_msb = bmp->reg(0xFA);
    bmp->setTemperature(50.0);
    uint8_t after_msb = bmp->reg(0xFA);
    TEST_ASSERT_NOT_EQUAL(before_msb, after_msb);
}

void test_pressure_registers_change_on_setPressure(void) {
    auto bmp = std::make_shared<FakeBMP280>();
    uint8_t before_msb = bmp->reg(0xF7);
    bmp->setPressure(50000.0);
    uint8_t after_msb = bmp->reg(0xF7);
    TEST_ASSERT_NOT_EQUAL(before_msb, after_msb);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_chip_id);
    RUN_TEST(test_calibration_coefficients_present);
    RUN_TEST(test_via_I2C_read_chip_id);
    RUN_TEST(test_temperature_registers_change_on_setTemperature);
    RUN_TEST(test_pressure_registers_change_on_setPressure);
    return UNITY_END();
}
