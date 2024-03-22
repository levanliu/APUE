#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

// Define the event structure
struct Event
{
    std::string data;
};

class CachedMessageQueue
{
public:
    void enqueue(const Event &event)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cache_.push(event);
        condition_.notify_one();
    }

    Event dequeue()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (cache_.empty())
        {
            condition_.wait(lock);
        }

        Event event = cache_.front();
        cache_.pop();
        return event;
    }

private:
    std::queue<Event> cache_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

int main()
{
    CachedMessageQueue messageQueue;

    // Producer thread
    std::thread producerThread([&messageQueue]()
                               {
        for (int i = 1; i <= 1000; ++i) {
            Event event;
            event.data = "Event " + std::to_string(i);
            messageQueue.enqueue(event);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } });

    // Consumer thread
    std::thread consumerThread([&messageQueue]()
                               {
        for (int i = 1; i <= 1000; ++i) {
            Event event = messageQueue.dequeue();
            std::cout << "Received event: " << event.data << std::endl;
        } });

    producerThread.join();
    consumerThread.join();

    return 0;
}
