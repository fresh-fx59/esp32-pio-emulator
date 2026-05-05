#include <Arduino.h>
#include <Wire.h>
#include <esp32sim_unity/esp32sim.h>
#include <peripherals/FakeDS3231.h>
#include <unity.h>

#include <memory>

void setUp(void) {
    esp32sim::Sim::reset();
    esp32sim::I2CBus::reset_all();
    esp32sim::Adc::instance().reset();
}
void tearDown(void) {}

void test_logs_after_setup_and_loop(void) {
    auto rtc = std::make_shared<esp32sim::peripherals::FakeDS3231>();
    rtc->setDate(15, 6, 2026);
    rtc->setTime(12, 30, 45);
    esp32sim::I2CBus::for_index(0).attach(0x68, rtc);

    esp32sim::Adc::instance().set_value(34, 2048);

    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();

    std::string out = esp32sim::Sim::uart(0).drainTx();
    TEST_ASSERT_TRUE(out.find("READY") != std::string::npos);
    TEST_ASSERT_TRUE(out.find("2026-06-15 12:30:45 moisture=2048") != std::string::npos);
}

void test_logs_again_after_interval(void) {
    auto rtc = std::make_shared<esp32sim::peripherals::FakeDS3231>();
    esp32sim::I2CBus::for_index(0).attach(0x68, rtc);
    esp32sim::Adc::instance().set_value(34, 1000);

    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    esp32sim::Sim::uart(0).drainTx();  // discard first line

    // Advance enough for the next interval and run loop again.
    esp32sim::Sim::advanceMs(11000);
    esp32sim::Sim::runLoop();
    std::string out = esp32sim::Sim::uart(0).drainTx();
    TEST_ASSERT_TRUE(out.find("moisture=1000") != std::string::npos);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_logs_after_setup_and_loop);
    RUN_TEST(test_logs_again_after_interval);
    return UNITY_END();
}
