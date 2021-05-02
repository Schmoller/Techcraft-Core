#pragma once

#include <deque>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <thread>
#include <vector>
#include <atomic>
#include <functional>
namespace Utilities {

// Forward declarations
class WorkerTask;
class Worker;
class WorkerFleet;


class WorkerTask {
    public:
    WorkerTask(std::function<void()> &&callback);

    void updateID(uint32_t id);
    uint32_t getID() const;

    void run();

    private:
    uint32_t id;

    std::function<void()> callback;
};

class Worker {
    public:
    Worker(WorkerFleet &fleet);
    Worker(Worker &other) = delete;
    Worker(Worker &&other);

    void shutdown();
    void join();

    private:
    void run();

    WorkerFleet &fleet;
    std::thread thread;

    bool isRunning;
};

/**
 * The worker fleet controls and provisions tasks to a fleet of thread workers.
 * Thread execution can be paused and resumed to allow for synchronisation.
 */
class WorkerFleet {
    friend class Worker;

    public:
    WorkerFleet(bool startBlocked, float coreUtilisationPercent = 1.0f);
    ~WorkerFleet();

    /**
     * Submits a task to be processed.
     * 
     * @param task The task to process
     * @returns The ID of the task
     */
    uint32_t submit(std::shared_ptr<WorkerTask> &task);

    /**
     * Cancels a submitted task
     * @param id The ID returned by submit()
     */
    void cancel(uint32_t id);
    /**
     * Cancels all tasks
     */
    void cancelAll();

    /**
     * Waits for all the worker threads to have processed their task queues
     * and stops new tasks from being executed yet.
     */
    void block();

    /**
     * Allows worker threads to start processing their task queues.
     */
    void unblock();

    /**
     * Terminates the worker fleet.
     * This will wait until all tasks are complete unless now is true
     * @param now When true, doesnt wait for tasks to complete
     */
    void shutdown(bool now = false);

    protected:
    std::shared_ptr<WorkerTask> popTask();

    private:

    std::vector<Worker> workers;
    bool isRunning;

    // The task queue
    std::mutex taskQueueLock;
    std::deque<std::shared_ptr<WorkerTask>> taskQueue;
    std::condition_variable submitNotification;
    uint32_t nextId;

    // Blocking vs unblocking
    std::condition_variable unblockNotification;
    std::condition_variable blockNotification;
    int blockedWorkers;
    bool isBlocked;
};

}