#ifndef CRAWLER_CONCURRENT_HPP
#define CRAWLER_CONCURRENT_HPP

#include <queue>
#include <mutex>
#include <unordered_set>

namespace NCrawler {

template<typename T>
class ConcurrentQueue
{
public:
	void push(const T& t)
	{
		std::lock_guard<std::mutex> lock(mutex);
		queue.push(t);
	}

	T pop()
	{
		std::lock_guard<std::mutex> lock(mutex);
		T value = queue.front();
		queue.pop();
		return value;
	}

	bool tryPop(T& t)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (queue.empty())
		{
			return false;
		}
		t = queue.front();
		queue.pop();
		return true;
	}

	T front() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return queue.front();
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return queue.empty();
	}

	size_t size() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return queue.size();
	}
private:
	mutable std::mutex mutex;
	std::queue<T> queue;
};

template<typename T>
class ConcurrentUnorderedSet
{
public:
	void insert(const T& t)
	{
		std::lock_guard<std::mutex> lock(mutex);
		set.insert(t);
	}

	bool tryInsert(const T& t)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (set.count(t) > 0)
		{
			return false;
		}
		set.insert(t);
		return true;
	}

	bool contains(const T& t) const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return (set.count(t) > 0);
	}

	std::vector<T> values()
	{
		return std::vector<T> (set.begin(), set.end());
	}

private:
	mutable std::mutex mutex;
	std::unordered_set<T> set;
};

} // namespace NCrawler

#endif // CRAWLER_CONCURRENT_HPP
