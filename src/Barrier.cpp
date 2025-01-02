#include "Barrier.h"


    // Default constructor
Barrier::Barrier() : count(0), total_count(0), initialized(false) {}

// Initialize with number of threads
void Barrier::initialize(size_t num_threads) {
    std::unique_lock<std::mutex> lock(mutex);
    if (initialized) {
        throw std::runtime_error("Barrier already initialized");
    }
    count = num_threads;
    total_count = num_threads;
    initialized = true;
}

void Barrier::wait() {
    std::unique_lock<std::mutex> lock(mutex);
    if (!initialized) {
        throw std::runtime_error("Barrier not initialized");
    }
    
    if (--count == 0) {
        count = total_count;
        cv.notify_all();
    } else {
        cv.wait(lock, [this] { return count == total_count; });
    }
}
    
    


