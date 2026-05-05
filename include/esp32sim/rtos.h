// include/esp32sim/rtos.h — cooperative FreeRTOS shim (T4 alpha)
//
// Per ADR D2 (T4 v0.2 spec), this is option (a) cooperative pseudo-
// scheduler. Tasks don't run in parallel; tests call run_task to drive
// a registered task function manually. Queues + semaphores are
// non-isolated in-memory primitives.
//
// What this catches: queue protocol bugs, task setup bugs, sequencing.
// What it doesn't: race conditions, real preemption, priority inversion.
#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace esp32sim {

class Rtos {
public:
    static Rtos& instance();
    void reset();

    // Task registration
    using TaskFn = std::function<void(void*)>;
    int register_task(const std::string& name, TaskFn fn, void* arg = nullptr);
    bool run_task_iteration(const std::string& name);  // calls fn(arg) once
    bool task_exists(const std::string& name) const;
    int task_count() const { return (int)tasks_.size(); }

    // Queue primitives — backed by std::deque<std::vector<uint8_t>>
    int  queue_create(int item_size, int max_items);
    bool queue_send(int handle, const void* item, int /*timeout*/);
    bool queue_receive(int handle, void* out, int /*timeout*/);
    int  queue_size(int handle) const;

    // Semaphore primitives — counting semaphore
    int  semaphore_create(int initial = 0, int max = 1);
    bool semaphore_take(int handle, int /*timeout*/);
    bool semaphore_give(int handle);
    int  semaphore_count(int handle) const;

private:
    Rtos() = default;
    struct Task {
        std::string name;
        TaskFn fn;
        void* arg;
    };
    std::map<std::string, Task> tasks_;

    struct Queue { int item_size; int max_items; std::deque<std::vector<uint8_t>> q; };
    std::vector<Queue> queues_;

    struct Semaphore { int count; int max; };
    std::vector<Semaphore> semaphores_;
};

}  // namespace esp32sim
