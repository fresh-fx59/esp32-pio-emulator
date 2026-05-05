#include <esp32sim/rtos.h>

#include <cstring>

namespace esp32sim {

Rtos& Rtos::instance() {
    static Rtos r;
    return r;
}

void Rtos::reset() {
    tasks_.clear();
    queues_.clear();
    semaphores_.clear();
}

int Rtos::register_task(const std::string& name, TaskFn fn, void* arg) {
    tasks_[name] = Task{name, std::move(fn), arg};
    return (int)tasks_.size();
}

bool Rtos::run_task_iteration(const std::string& name) {
    auto it = tasks_.find(name);
    if (it == tasks_.end()) return false;
    it->second.fn(it->second.arg);
    return true;
}

bool Rtos::task_exists(const std::string& name) const {
    return tasks_.count(name) > 0;
}

int Rtos::queue_create(int item_size, int max_items) {
    queues_.push_back({item_size, max_items, {}});
    return (int)queues_.size() - 1;
}

bool Rtos::queue_send(int handle, const void* item, int /*timeout*/) {
    if (handle < 0 || handle >= (int)queues_.size()) return false;
    auto& q = queues_[handle];
    if ((int)q.q.size() >= q.max_items) return false;
    std::vector<uint8_t> v(q.item_size);
    std::memcpy(v.data(), item, q.item_size);
    q.q.push_back(std::move(v));
    return true;
}

bool Rtos::queue_receive(int handle, void* out, int /*timeout*/) {
    if (handle < 0 || handle >= (int)queues_.size()) return false;
    auto& q = queues_[handle];
    if (q.q.empty()) return false;
    std::memcpy(out, q.q.front().data(), q.item_size);
    q.q.pop_front();
    return true;
}

int Rtos::queue_size(int handle) const {
    if (handle < 0 || handle >= (int)queues_.size()) return 0;
    return (int)queues_[handle].q.size();
}

int Rtos::semaphore_create(int initial, int max) {
    semaphores_.push_back({initial, max});
    return (int)semaphores_.size() - 1;
}

bool Rtos::semaphore_take(int handle, int /*timeout*/) {
    if (handle < 0 || handle >= (int)semaphores_.size()) return false;
    auto& s = semaphores_[handle];
    if (s.count <= 0) return false;
    s.count--;
    return true;
}

bool Rtos::semaphore_give(int handle) {
    if (handle < 0 || handle >= (int)semaphores_.size()) return false;
    auto& s = semaphores_[handle];
    if (s.count >= s.max) return false;
    s.count++;
    return true;
}

int Rtos::semaphore_count(int handle) const {
    if (handle < 0 || handle >= (int)semaphores_.size()) return 0;
    return semaphores_[handle].count;
}

}  // namespace esp32sim
