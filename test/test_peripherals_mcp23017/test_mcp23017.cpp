#include <Wire.h>
#include <esp32sim/i2c.h>
#include <peripherals/FakeMCP23017.h>
#include <unity.h>

#include <memory>

using namespace esp32sim;
using esp32sim::peripherals::FakeMCP23017;

void setUp(void) { I2CBus::reset_all(); }
void tearDown(void) {}

void test_default_iodir_all_inputs(void) {
    auto exp = std::make_shared<FakeMCP23017>();
    TEST_ASSERT_EQUAL_UINT8(0xFF, exp->reg(0x00));
    TEST_ASSERT_EQUAL_UINT8(0xFF, exp->reg(0x01));
}

void test_setMode_modifies_iodir(void) {
    auto exp = std::make_shared<FakeMCP23017>();
    exp->setMode(0, false);   // pin 0 = output
    exp->setMode(8, false);   // pin 8 = port B pin 0 = output
    TEST_ASSERT_EQUAL_UINT8(0xFE, exp->reg(0x00));  // bit 0 cleared
    TEST_ASSERT_EQUAL_UINT8(0xFE, exp->reg(0x01));  // bit 0 cleared on port B
}

void test_setPin_writes_to_GPIO_register(void) {
    auto exp = std::make_shared<FakeMCP23017>();
    exp->setPin(3, 1);  // pin 3 high
    TEST_ASSERT_EQUAL_UINT8(0x08, exp->reg(0x12));  // bit 3 of GPIOA
    exp->setPin(11, 1);  // pin 11 = port B pin 3 high
    TEST_ASSERT_EQUAL_UINT8(0x08, exp->reg(0x13));
}

void test_via_I2C_write_GPIO_then_read(void) {
    auto exp = std::make_shared<FakeMCP23017>();
    I2CBus::for_index(0).attach(0x20, exp);

    // Master writes to GPIOA: set bit 5 high
    Wire.beginTransmission(0x20);
    Wire.write((uint8_t)0x12);
    Wire.write((uint8_t)0x20);  // bit 5 high
    Wire.endTransmission();

    // Master reads GPIOA back
    Wire.beginTransmission(0x20);
    Wire.write((uint8_t)0x12);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x20, (size_t)1);
    TEST_ASSERT_EQUAL_INT(0x20, Wire.read());
}

void test_writing_GPIO_updates_OLAT(void) {
    auto exp = std::make_shared<FakeMCP23017>();
    I2CBus::for_index(0).attach(0x20, exp);

    Wire.beginTransmission(0x20);
    Wire.write((uint8_t)0x12);
    Wire.write((uint8_t)0xAA);
    Wire.endTransmission();

    TEST_ASSERT_EQUAL_UINT8(0xAA, exp->reg(0x14));  // OLATA mirrors GPIOA write
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_default_iodir_all_inputs);
    RUN_TEST(test_setMode_modifies_iodir);
    RUN_TEST(test_setPin_writes_to_GPIO_register);
    RUN_TEST(test_via_I2C_write_GPIO_then_read);
    RUN_TEST(test_writing_GPIO_updates_OLAT);
    return UNITY_END();
}
