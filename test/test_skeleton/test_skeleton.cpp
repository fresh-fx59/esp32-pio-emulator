// test/test_skeleton/test_skeleton.cpp
//
// T0 skeleton smoke test. Proves PlatformIO's native env + Unity work in this
// repo. Intentionally trivial — no framework code is exercised yet. T1 replaces
// this with real GPIO TDD tests.

#include <unity.h>

void setUp(void) {
    // Runs before every test. Empty for now.
}

void tearDown(void) {
    // Runs after every test. Empty for now.
}

void test_skeleton_passes(void) {
    TEST_ASSERT_EQUAL_INT(1, 1);
}

void test_native_env_macro_defined(void) {
    // Sanity check that platformio.ini's build_flags reach the test binary.
#ifndef ESP32_PIO_EMULATOR_NATIVE
    TEST_FAIL_MESSAGE("ESP32_PIO_EMULATOR_NATIVE macro not defined "
                      "— platformio.ini build_flags are not reaching the test binary");
#endif
    TEST_PASS();
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_skeleton_passes);
    RUN_TEST(test_native_env_macro_defined);
    return UNITY_END();
}
