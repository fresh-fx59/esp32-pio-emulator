// include/Preferences.h — fake arduino-esp32 Preferences (T4)
#pragma once

#include <stdint.h>
#include <string>

#ifdef __cplusplus

class Preferences {
public:
    bool begin(const char* ns, bool /*read_only*/ = false);
    void end() { opened_ = false; }
    bool clear();

    size_t putString(const char* key, const char* value);
    size_t putString(const char* key, const std::string& value) {
        return putString(key, value.c_str());
    }
    std::string getString(const char* key, const std::string& def = "");

    size_t putUInt(const char* key, uint32_t value);
    uint32_t getUInt(const char* key, uint32_t def = 0);

    size_t putInt(const char* key, int32_t value);
    int32_t getInt(const char* key, int32_t def = 0);

    size_t putBool(const char* key, bool value);
    bool getBool(const char* key, bool def = false);

    bool remove(const char* key);
    bool isKey(const char* key);

private:
    std::string ns_;
    bool opened_ = false;
};

#endif  // __cplusplus
