#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

// -------------------------------------------------------------------------
// ThreadPool — fixed set of worker threads draining a shared job queue.
// -------------------------------------------------------------------------
class ThreadPool
{
public:
    explicit ThreadPool(size_t threadCount)
    {
        for (size_t i = 0; i < threadCount; ++i)
        {
            m_Workers.emplace_back([this]()
            {
                while (true)
                {
                    std::function<void()> job;
                    {
                        std::unique_lock lock(m_Mutex);
                        m_CV.wait(lock, [this] { return m_Shutdown || !m_Queue.empty(); });
                        if (m_Shutdown && m_Queue.empty())
                            return;
                        job = std::move(m_Queue.front());
                        m_Queue.pop();
                    }
                    job();
                }
            });
        }
    }

    ~ThreadPool()
    {
        {
            std::lock_guard lock(m_Mutex);
            m_Shutdown = true;
        }
        m_CV.notify_all();
        for (auto& t : m_Workers)
            t.join();
    }

    void Submit(std::function<void()> job)
    {
        {
            std::lock_guard lock(m_Mutex);
            m_Queue.push(std::move(job));
        }
        m_CV.notify_one();
    }

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    std::vector<std::thread>          m_Workers;
    std::queue<std::function<void()>> m_Queue;
    std::mutex                        m_Mutex;
    std::condition_variable           m_CV;
    bool                              m_Shutdown = false;
};

#endif // THREAD_POOL_H
