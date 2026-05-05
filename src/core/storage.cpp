#include <esp32sim/storage.h>

namespace esp32sim {

// -------- Nvs --------

Nvs& Nvs::instance() {
    static Nvs n;
    return n;
}

void Nvs::reset() { data_.clear(); }

void Nvs::set_string(const std::string& ns, const std::string& key, const std::string& v) {
    auto& slot = data_[ns][key];
    slot.kind = Value::Kind::STRING;
    slot.s = v;
}
bool Nvs::get_string(const std::string& ns, const std::string& key, std::string& out) const {
    auto nit = data_.find(ns);
    if (nit == data_.end()) return false;
    auto kit = nit->second.find(key);
    if (kit == nit->second.end() || kit->second.kind != Value::Kind::STRING) return false;
    out = kit->second.s;
    return true;
}

void Nvs::set_uint(const std::string& ns, const std::string& key, uint32_t v) {
    auto& slot = data_[ns][key];
    slot.kind = Value::Kind::UINT;
    slot.u = v;
}
bool Nvs::get_uint(const std::string& ns, const std::string& key, uint32_t& out) const {
    auto nit = data_.find(ns);
    if (nit == data_.end()) return false;
    auto kit = nit->second.find(key);
    if (kit == nit->second.end() || kit->second.kind != Value::Kind::UINT) return false;
    out = kit->second.u;
    return true;
}

void Nvs::set_int(const std::string& ns, const std::string& key, int32_t v) {
    auto& slot = data_[ns][key];
    slot.kind = Value::Kind::INT;
    slot.i = v;
}
bool Nvs::get_int(const std::string& ns, const std::string& key, int32_t& out) const {
    auto nit = data_.find(ns);
    if (nit == data_.end()) return false;
    auto kit = nit->second.find(key);
    if (kit == nit->second.end() || kit->second.kind != Value::Kind::INT) return false;
    out = kit->second.i;
    return true;
}

void Nvs::set_bool(const std::string& ns, const std::string& key, bool v) {
    auto& slot = data_[ns][key];
    slot.kind = Value::Kind::BOOL;
    slot.b = v;
}
bool Nvs::get_bool(const std::string& ns, const std::string& key, bool& out) const {
    auto nit = data_.find(ns);
    if (nit == data_.end()) return false;
    auto kit = nit->second.find(key);
    if (kit == nit->second.end() || kit->second.kind != Value::Kind::BOOL) return false;
    out = kit->second.b;
    return true;
}

bool Nvs::remove(const std::string& ns, const std::string& key) {
    auto nit = data_.find(ns);
    if (nit == data_.end()) return false;
    return nit->second.erase(key) > 0;
}

void Nvs::clear_namespace(const std::string& ns) {
    data_.erase(ns);
}

bool Nvs::has(const std::string& ns, const std::string& key) const {
    auto nit = data_.find(ns);
    if (nit == data_.end()) return false;
    return nit->second.count(key) > 0;
}

// -------- FileSystem --------

FileSystem& FileSystem::instance() {
    static FileSystem fs;
    return fs;
}

void FileSystem::reset() { files_.clear(); }

bool FileSystem::write_file(const std::string& path, const std::string& content) {
    files_[path] = content;
    return true;
}

bool FileSystem::read_file(const std::string& path, std::string& out) const {
    auto it = files_.find(path);
    if (it == files_.end()) return false;
    out = it->second;
    return true;
}

bool FileSystem::exists(const std::string& path) const {
    return files_.count(path) > 0;
}

bool FileSystem::remove(const std::string& path) {
    return files_.erase(path) > 0;
}

bool FileSystem::mkdir(const std::string& /*path*/) {
    // In-memory FS doesn't track dirs explicitly; mkdir is a no-op success.
    return true;
}

std::vector<std::string> FileSystem::list_dir(const std::string& path) const {
    std::vector<std::string> out;
    std::string prefix = path;
    if (!prefix.empty() && prefix.back() != '/') prefix += '/';
    for (const auto& [k, _] : files_) {
        if (k.rfind(prefix, 0) == 0) out.push_back(k);
    }
    return out;
}

size_t FileSystem::file_size(const std::string& path) const {
    auto it = files_.find(path);
    return (it == files_.end()) ? 0 : it->second.size();
}

}  // namespace esp32sim
