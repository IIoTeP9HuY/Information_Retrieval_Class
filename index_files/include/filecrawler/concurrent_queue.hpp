#ifndef CONCURRENT_QUEUE_HPP
#define CONCURRENT_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <boost/optional.hpp>

#include "logger.hpp"

using namespace logging;

namespace filecrawler
{

template <typename T>
class ConcurrentQueue
{
public:
    ConcurrentQueue(): timeout(std::chrono::milliseconds(100)), queueIsNotEmptyPredicate(queue)
    {
    }

    ~ConcurrentQueue()
    {
    }

    void push(const T& element)
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push(element);
        queueIsNotEmpty.notify_one();
    }

    void pop()
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (queue.empty())
        {
            Log::error("Can't pop from empty queue");
        }
        else
        {
            queue.pop();
        }
    }

    struct QueueIsNotEmptyPredicate
    {
        QueueIsNotEmptyPredicate(const std::queue<std::string>& queue): queue(queue) {}

        bool operator() () const
        {
            return !queue.empty();
        }

        const std::queue<std::string>& queue;
    };

    boost::optional<T> front() const
    {
        std::unique_lock<std::mutex> lock(queueMutex);
//        bool waitResult = queueIsNotEmpty.wait_for(lock, timeout, [&] { return !queue.empty(); });
        bool waitResult = queueIsNotEmpty.wait_for(lock, timeout, queueIsNotEmptyPredicate);
        boost::optional<T> result;
        if (waitResult)
        {
            result = queue.front();
        }
        return result;
    }

    boost::optional<T> pull()
    {
        std::unique_lock<std::mutex> lock(queueMutex);
//        bool waitResult = queueIsNotEmpty.wait_for(lock, timeout, [&] { return !queue.empty(); });
        bool waitResult = queueIsNotEmpty.wait_for(lock, timeout, queueIsNotEmptyPredicate);
        boost::optional<T> result;
        if (waitResult)
        {
            result = queue.front();
            queue.pop();
        }
        return result;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        return queue.empty();
    }

private:
    std::queue<T> queue;
    std::condition_variable queueIsNotEmpty;
    std::chrono::milliseconds timeout;
    QueueIsNotEmptyPredicate queueIsNotEmptyPredicate;
    mutable std::mutex queueMutex;
};

} // namespace filecrawler

#endif // CONCURRENT_QUEUE_HPP
