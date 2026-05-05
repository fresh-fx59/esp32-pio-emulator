#include <Arduino.h>
#include <esp32sim/adc.h>
#include <esp32sim/pwm.h>
#include <unity.h>

void setUp(void) {
    esp32sim::Adc::instance().reset();
    esp32sim::Pwm::instance().reset();
}
void tearDown(void) {}

// -------- ADC --------

void test_adc_default_zero(void) {
    TEST_ASSERT_EQUAL_INT(0, analogRead(34));
}

void test_adc_set_then_read(void) {
    esp32sim::Adc::instance().set_value(34, 2048);
    TEST_ASSERT_EQUAL_INT(2048, analogRead(34));
}

void test_adc_resolution_change(void) {
    analogReadResolution(10);
    TEST_ASSERT_EQUAL_INT(10, esp32sim::Adc::instance().resolution());
}

void test_adc_pin_attenuation(void) {
    analogSetPinAttenuation(34, 3);
    TEST_ASSERT_EQUAL_INT(3, esp32sim::Adc::instance().attenuation(34));
}

void test_adc_global_attenuation_applies_to_all_pins(void) {
    analogSetAttenuation(2);
    TEST_ASSERT_EQUAL_INT(2, esp32sim::Adc::instance().attenuation(34));
    TEST_ASSERT_EQUAL_INT(2, esp32sim::Adc::instance().attenuation(15));
}

// -------- PWM --------

void test_pwm_setup_records_freq_and_resolution(void) {
    ledcSetup(0, 5000, 12);
    TEST_ASSERT_EQUAL_UINT32(5000, esp32sim::Pwm::instance().frequency(0));
    TEST_ASSERT_EQUAL_INT(12, esp32sim::Pwm::instance().resolution(0));
    TEST_ASSERT_TRUE(esp32sim::Pwm::instance().in_use(0));
}

void test_pwm_attach_and_write(void) {
    ledcSetup(0, 1000, 8);
    ledcAttachPin(5, 0);
    ledcWrite(0, 128);
    TEST_ASSERT_EQUAL_INT(5, esp32sim::Pwm::instance().pin(0));
    TEST_ASSERT_EQUAL_UINT32(128, esp32sim::Pwm::instance().duty(0));
}

void test_pwm_ledcRead_returns_current_duty(void) {
    ledcSetup(0, 1000, 8);
    ledcWrite(0, 42);
    TEST_ASSERT_EQUAL_UINT32(42, ledcRead(0));
}

void test_analogWrite_uses_default_channel(void) {
    analogWrite(5, 200);
    TEST_ASSERT_EQUAL_UINT32(200, esp32sim::Pwm::instance().duty(0));
    TEST_ASSERT_EQUAL_INT(5, esp32sim::Pwm::instance().pin(0));
}

void test_analogWrite_clamps_to_0_255(void) {
    analogWrite(5, -10);
    TEST_ASSERT_EQUAL_UINT32(0, esp32sim::Pwm::instance().duty(0));
    analogWrite(5, 999);
    TEST_ASSERT_EQUAL_UINT32(255, esp32sim::Pwm::instance().duty(0));
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_adc_default_zero);
    RUN_TEST(test_adc_set_then_read);
    RUN_TEST(test_adc_resolution_change);
    RUN_TEST(test_adc_pin_attenuation);
    RUN_TEST(test_adc_global_attenuation_applies_to_all_pins);
    RUN_TEST(test_pwm_setup_records_freq_and_resolution);
    RUN_TEST(test_pwm_attach_and_write);
    RUN_TEST(test_pwm_ledcRead_returns_current_duty);
    RUN_TEST(test_analogWrite_uses_default_channel);
    RUN_TEST(test_analogWrite_clamps_to_0_255);
    return UNITY_END();
}
