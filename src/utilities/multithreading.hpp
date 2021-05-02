#pragma once

#include <queue>
#include <vector>

#include <mutex>
#include <condition_variable>

namespace Utilities {

template <typename T>
class BlockingQueue {
    public:

    /**
     * Pushes a value onto the queue
     */
    void push(const T &value) {
        std::unique_lock<std::mutex> localLock(lock);
        backingQueue.push(value);
        condition.notify_all();
    }
    /**
     * Retrieves a value from the queue, waiting until there is a value to retrieve.
     */
    T pop() {
        std::unique_lock<std::mutex> localLock(lock);
        while (backingQueue.empty()) {
            condition.wait(localLock);
        }

        T value = backingQueue.front();
        backingQueue.pop();
        return value;
    }

    /**
     * Checks if the queue is empty.
     * Note: This may not be accurate
     */
    bool empty() {
        std::unique_lock<std::mutex> localLock(lock);
        return backingQueue.empty();
    }
    /**
     * Checks the size of the queue
     * Note: This may not be accurate
     */
    size_t size() {
        std::unique_lock<std::mutex> localLock(lock);
        return backingQueue.size();
    }

    /**
     * Clears the queue
     */
    void clear() {
        std::unique_lock<std::mutex> localLock(lock);
        while (!backingQueue.empty()) {
            backingQueue.pop();
        }
    }

    private:
    std::queue<T> backingQueue;
    std::mutex lock;
    std::condition_variable condition;
};

template <typename T>
class SharedVector {
    public:
    /**
     * Captures the contents of the vector so they can be
     * used.
     */
    std::vector<T> capture() {
        std::unique_lock<std::mutex> localLock(lock);
        std::vector<T> data = std::move(internalVector);
        internalVector = std::vector<T>();
        return data;
    }

    void pushBack(const T &value) {
        std::unique_lock<std::mutex> localLock(lock);
        internalVector.push_back(value);
    }

    bool empty() {
        std::unique_lock<std::mutex> localLock(lock);
        return internalVector.empty();
    }

    size_t size() {
        std::unique_lock<std::mutex> localLock(lock);
        return internalVector.size();
    }

    private:
    std::vector<T> internalVector;
    std::mutex lock;
};
}