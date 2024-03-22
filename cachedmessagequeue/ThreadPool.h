#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool
{
public:
    using Task = std::function<void()>;

    explicit ThreadPool(std::size_t numThreads)
        : mStop(false), mNumThreads(numThreads)
    {
        for (std::size_t i = 0; i < numThreads; i++)
        {
            mWorkers.emplace_back([this]
                                  {
                while (true)
                {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock{mQueueMutex};
                        mCondVar.wait(lock, [this] { return mStop || !mTasks.empty(); });
                        if (mStop && mTasks.empty())
                            return;
                        task = std::move(mTasks.front());
                        mTasks.pop();
                    }
                    task();
                } });
        }

        mManager = std::thread([this]
                               {
            while (!mStop)
            {
                std::unique_lock<std::mutex> lock{mQueueMutex};
                mCondVar.wait_for(lock, std::chrono::seconds(1));
                if (mStop)
                    return;

                auto tasksSize = mTasks.size();
                auto workersSize = mWorkers.size();
                auto threadsRatio = tasksSize / (workersSize + 1);

                if (tasksSize > workersSize && threadsRatio > 2)
                {
                    std::cout << "Adding new worker thread\n";
                    mWorkers.emplace_back([this] {
                        while (true)
                        {
                            Task task;
                            {
                                std::unique_lock<std::mutex> lock{mQueueMutex};
                                mCondVar.wait(lock, [this] { return mStop || !mTasks.empty(); });
                                if (mStop && mTasks.empty())
                                    return;
                                task = std::move(mTasks.front());
                                mTasks.pop();
                            }
                            task();
                        }
                    });
                }
                else if (tasksSize < workersSize / 2 && workersSize > mNumThreads)
                {
                    std::cout << "Removing worker thread\n";
                    mWorkers.back().detach();
                    mWorkers.pop_back();
                }
            } });
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock{mQueueMutex};
            mStop = true;
        }
        mCondVar.notify_all();
        mManager.join();
        for (auto &worker : mWorkers)
            if (worker.joinable())
                worker.join();
    }

    template <typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock{mQueueMutex};
            if (mStop)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            mTasks.emplace([task]
                           { (*task)(); });
        }
        mCondVar.notify_one();
        return res;
    }

private:
    std::vector<std::thread> mWorkers;
    std::queue<Task> mTasks;
    std::thread mManager;
    std::mutex mQueueMutex;
    std::condition_variable mCondVar;
    bool mStop;
    std::size_t mNumThreads;
};