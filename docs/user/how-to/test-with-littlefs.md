# How to test a sketch using the filesystem

Your sketch uses LittleFS or SPIFFS to read/write files. The sim provides an
in-memory filesystem accessible via `esp32sim::FileSystem`.

## Pattern

```cpp
#include <esp32sim_unity/esp32sim.h>

void test_writes_log_to_filesystem(void) {
    esp32sim::Sim::reset();
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();

    auto& fs = esp32sim::FileSystem::instance();
    TEST_ASSERT_TRUE(fs.exists("/log.txt"));
    std::string content;
    fs.read_file("/log.txt", content);
    TEST_ASSERT_TRUE(content.find("BOOT") != std::string::npos);
}

void test_reads_seeded_config(void) {
    esp32sim::Sim::reset();
    esp32sim::FileSystem::instance().write_file("/config.json", "{\"x\":1}");
    esp32sim::Sim::runSetup();
    // sketch should have read /config.json successfully
}
```

## Notes

The fake filesystem is a flat path-to-content map. `mkdir` is a no-op success
because there's no directory tree per se — paths can have any nesting and
`list_dir(prefix)` returns matching keys.

## What this catches / doesn't

**Catches:** path bugs, missing-file handling, content roundtrip, filesystem
state across deep-sleep cycles (the in-memory FS persists across `Sim::reset()`
isn't called).

**Doesn't:** real flash wear-leveling, partition boundaries, fsck edge cases,
filesystem corruption scenarios.
