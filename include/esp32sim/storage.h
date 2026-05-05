// include/esp32sim/storage.h — NVS + filesystem (T4)
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace esp32sim {

// In-memory NVS / Preferences backend
class Nvs {
public:
    static Nvs& instance();
    void reset();

    void set_string(const std::string& ns, const std::string& key, const std::string& value);
    bool get_string(const std::string& ns, const std::string& key, std::string& out) const;

    void set_uint(const std::string& ns, const std::string& key, uint32_t value);
    bool get_uint(const std::string& ns, const std::string& key, uint32_t& out) const;

    void set_int(const std::string& ns, const std::string& key, int32_t value);
    bool get_int(const std::string& ns, const std::string& key, int32_t& out) const;

    void set_bool(const std::string& ns, const std::string& key, bool value);
    bool get_bool(const std::string& ns, const std::string& key, bool& out) const;

    bool remove(const std::string& ns, const std::string& key);
    void clear_namespace(const std::string& ns);
    bool has(const std::string& ns, const std::string& key) const;

private:
    Nvs() = default;
    struct Value {
        enum class Kind { STRING, UINT, INT, BOOL };
        Kind kind = Kind::STRING;
        std::string s;
        uint32_t u = 0;
        int32_t i = 0;
        bool b = false;
    };
    std::map<std::string, std::map<std::string, Value>> data_;
};

// In-memory filesystem (LittleFS / SPIFFS-compatible behavior)
class FileSystem {
public:
    static FileSystem& instance();
    void reset();

    bool write_file(const std::string& path, const std::string& content);
    bool read_file(const std::string& path, std::string& out) const;
    bool exists(const std::string& path) const;
    bool remove(const std::string& path);
    bool mkdir(const std::string& path);
    std::vector<std::string> list_dir(const std::string& path) const;
    size_t file_size(const std::string& path) const;

private:
    FileSystem() = default;
    std::map<std::string, std::string> files_;
};

}  // namespace esp32sim
