#include <Preferences.h>
#include <esp32sim/storage.h>
#include <unity.h>

void setUp(void) {
    esp32sim::Nvs::instance().reset();
    esp32sim::FileSystem::instance().reset();
}
void tearDown(void) {}

// -------- NVS --------

void test_nvs_string_roundtrip(void) {
    auto& nvs = esp32sim::Nvs::instance();
    nvs.set_string("ns", "key", "hello");
    std::string out;
    TEST_ASSERT_TRUE(nvs.get_string("ns", "key", out));
    TEST_ASSERT_EQUAL_STRING("hello", out.c_str());
}

void test_nvs_missing_key_returns_false(void) {
    auto& nvs = esp32sim::Nvs::instance();
    std::string out;
    TEST_ASSERT_FALSE(nvs.get_string("ns", "missing", out));
}

void test_nvs_namespaces_isolated(void) {
    auto& nvs = esp32sim::Nvs::instance();
    nvs.set_uint("a", "k", 1);
    nvs.set_uint("b", "k", 2);
    uint32_t v = 0;
    nvs.get_uint("a", "k", v); TEST_ASSERT_EQUAL_UINT32(1, v);
    nvs.get_uint("b", "k", v); TEST_ASSERT_EQUAL_UINT32(2, v);
}

// -------- Preferences --------

void test_preferences_putString_getString(void) {
    Preferences prefs;
    prefs.begin("config");
    prefs.putString("ssid", "MyNetwork");
    TEST_ASSERT_EQUAL_STRING("MyNetwork", prefs.getString("ssid").c_str());
    TEST_ASSERT_EQUAL_STRING("default", prefs.getString("missing", "default").c_str());
}

void test_preferences_putUInt_getUInt(void) {
    Preferences prefs;
    prefs.begin("config");
    prefs.putUInt("count", 42);
    TEST_ASSERT_EQUAL_UINT32(42, prefs.getUInt("count"));
}

void test_preferences_putBool_getBool(void) {
    Preferences prefs;
    prefs.begin("config");
    prefs.putBool("enabled", true);
    TEST_ASSERT_TRUE(prefs.getBool("enabled"));
    TEST_ASSERT_FALSE(prefs.getBool("missing"));
}

void test_preferences_clear(void) {
    Preferences prefs;
    prefs.begin("config");
    prefs.putString("k", "v");
    prefs.clear();
    TEST_ASSERT_FALSE(prefs.isKey("k"));
}

void test_preferences_persists_across_instances(void) {
    {
        Preferences p1;
        p1.begin("config");
        p1.putString("k", "persisted");
        p1.end();
    }
    {
        Preferences p2;
        p2.begin("config");
        TEST_ASSERT_EQUAL_STRING("persisted", p2.getString("k").c_str());
    }
}

// -------- FileSystem --------

void test_fs_write_and_read(void) {
    auto& fs = esp32sim::FileSystem::instance();
    TEST_ASSERT_TRUE(fs.write_file("/config.json", "{\"key\":1}"));
    std::string out;
    TEST_ASSERT_TRUE(fs.read_file("/config.json", out));
    TEST_ASSERT_EQUAL_STRING("{\"key\":1}", out.c_str());
}

void test_fs_exists_and_remove(void) {
    auto& fs = esp32sim::FileSystem::instance();
    fs.write_file("/x", "hello");
    TEST_ASSERT_TRUE(fs.exists("/x"));
    fs.remove("/x");
    TEST_ASSERT_FALSE(fs.exists("/x"));
}

void test_fs_list_dir(void) {
    auto& fs = esp32sim::FileSystem::instance();
    fs.write_file("/data/a.txt", "a");
    fs.write_file("/data/b.txt", "b");
    fs.write_file("/other.txt", "o");
    auto items = fs.list_dir("/data");
    TEST_ASSERT_EQUAL_size_t(2, items.size());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_nvs_string_roundtrip);
    RUN_TEST(test_nvs_missing_key_returns_false);
    RUN_TEST(test_nvs_namespaces_isolated);
    RUN_TEST(test_preferences_putString_getString);
    RUN_TEST(test_preferences_putUInt_getUInt);
    RUN_TEST(test_preferences_putBool_getBool);
    RUN_TEST(test_preferences_clear);
    RUN_TEST(test_preferences_persists_across_instances);
    RUN_TEST(test_fs_write_and_read);
    RUN_TEST(test_fs_exists_and_remove);
    RUN_TEST(test_fs_list_dir);
    return UNITY_END();
}
