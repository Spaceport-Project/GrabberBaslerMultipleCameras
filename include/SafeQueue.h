
// // Created by Adam Kecskes
// // https://github.com/K-Adam/SafeQueue

// #pragma once

// #include <mutex>
// #include <condition_variable>

// #include <queue>
// #include <utility>

// template<class T>
// class SafeQueue {

// 	std::queue<T> q;

// 	std::mutex mtx;
// 	std::condition_variable cv;

// 	std::condition_variable sync_wait;
// 	bool finish_processing = false;
// 	int sync_counter = 0;

// 	void DecreaseSyncCounter() {
// 		if (--sync_counter == 0) {
// 			sync_wait.notify_one();
// 		}
// 	}

// public:

// 	typedef typename std::queue<T>::size_type size_type;

// 	SafeQueue() {}

// 	~SafeQueue() {
// 		Finish();
// 	}

// 	void Produce(T&& item) {

// 		std::lock_guard<std::mutex> lock(mtx);

// 		q.push(std::move(item));
// 		cv.notify_one();

// 	}

// 	size_type Size() {

// 		std::lock_guard<std::mutex> lock(mtx);

// 		return q.size();

// 	}

// 	[[nodiscard]]
// 	bool Consume(T& item) {

// 		std::lock_guard<std::mutex> lock(mtx);

// 		if (q.empty()) {
// 			return false;
// 		}

// 		item = std::move(q.front());
// 		q.pop();

// 		return true;

// 	}

// 	[[nodiscard]]
// 	bool ConsumeSync(T& item) {

// 		std::unique_lock<std::mutex> lock(mtx);

// 		sync_counter++;

// 		cv.wait(lock, [&] {
// 			return !q.empty() || finish_processing;
// 		});

// 		if (q.empty()) {
// 			DecreaseSyncCounter();
// 			return false;
// 		}

// 		item = std::move(q.front());
// 		q.pop();

// 		DecreaseSyncCounter();
// 		return true;

// 	}

// 	void Finish() {

// 		std::unique_lock<std::mutex> lock(mtx);

// 		finish_processing = true;
// 		cv.notify_all();

// 		sync_wait.wait(lock, [&]() {
// 			return sync_counter == 0;
// 		});

// 		finish_processing = false;

// 	}

// };

#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <condition_variable>
#include <mutex>
#include <queue>


template <class T>
class SafeQueue
{
public:
    SafeQueue() : q(), m(), c() {}

    // Copy constructor
    SafeQueue(const SafeQueue& other)
        : q(other.q), m(), c() {}

    // Copy assignment operator
    SafeQueue& operator=(const SafeQueue& other)
    {
        if (&other != this) {
            // Lock both mutexes without deadlock
            std::lock(m, other.m);

            // Use std::lock_guard to manage the mutexes
            std::lock_guard<std::mutex> lock1(m, std::adopt_lock);
            std::lock_guard<std::mutex> other_lock(other.m, std::adopt_lock);

            // Copy the queue from 'other'
            q = other.q;

            // The mutex and condition variable are not copied
        }
        return *this;
    }

    // Add an element to the queue.
    void enqueue(T t)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(t);
        c.notify_one();
    }

    // Get the front element.
    // If the queue is empty, wait till a element is avaiable.
    T dequeue(void)
    {
        std::unique_lock<std::mutex> lock(m);
        while (q.empty())
        {
            // release lock as long as the wait and reaquire it afterwards.
            c.wait(lock);
        }
        T val = q.front();
        q.pop();
        return val;
    }
    int size() {
        std::unique_lock<std::mutex> lock(m);

        return q.size();
    }

private:
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;
};

#endif

