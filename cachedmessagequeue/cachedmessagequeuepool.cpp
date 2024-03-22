#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <future>
#include "ThreadPool.h"

struct Event
{
    std::string data;
};

class CachedMessageQueue
{
public:
    void enqueue(Event event)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cache_.push(std::move(event));
        }
        condition_.notify_one();
    }

    Event dequeue()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]
                        { return !cache_.empty(); });

        Event event = std::move(cache_.top());
        cache_.pop();
        return event;
    }
    struct EventComparator
    {
        bool operator()(const Event &event1, const Event &event2) const
        {
            // Sort based on the numeric value of event.data
            int number1 = std::stoi(event1.data.substr(6));
            int number2 = std::stoi(event2.data.substr(6));
            return number1 > number2; // Change to '<' for ascending order
        }
    };
    std::priority_queue<Event, std::vector<Event>, EventComparator> cache_;

private:
    std::mutex mutex_;
    std::condition_variable condition_;
};

int main()
{
    const size_t numThreads = 4;
    ThreadPool threadPool(numThreads);
    CachedMessageQueue messageQueue;

    std::thread producerThread([&messageQueue, &threadPool]()
                               {
        for (int i = 1; i <= 100; ++i) {
            Event event;
            event.data = "Event " + std::to_string(i);
            threadPool.enqueue([event = std::move(event), &messageQueue]() mutable {
                messageQueue.enqueue(std::move(event));
            });
        } });

    producerThread.join();

    while (!messageQueue.cache_.empty())
    {
        auto cur = messageQueue.cache_.top();
        std::cout << cur.data << std::endl;
        messageQueue.cache_.pop();
    }

    // std::thread consumerThread([&messageQueue, &threadPool]()
    //                            {
    //     for (int i = 1; i <= 100; ++i)
    //     {
    //         threadPool.enqueue([&messageQueue]() {
    //             std::cout << messageQueue.cache_.size() << std::endl;
    //             Event event = messageQueue.dequeue();
    //             std::cout << event.data << std::endl;
    //         });
    //     } });
    // consumerThread.join();

    return 0;
}
