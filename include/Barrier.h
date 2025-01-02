#include <mutex>
#include <condition_variable>
class Barrier {
private:
    std::mutex mutex;
    std::condition_variable cv;
    size_t count;
    size_t total_count;
    bool initialized;
    
public:
    // Default constructor
    Barrier();
    void initialize(size_t num_threads);
    void wait();
};
