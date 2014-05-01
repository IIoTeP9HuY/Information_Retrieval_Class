#ifndef CRAWLER_TIMER_HPP
#define CRAWLER_TIMER_HPP

#include <chrono>

class Timer
{
public:
	Timer(std::string title): title(title), start(std::chrono::high_resolution_clock::now()) {}
	
	void restart()
	{
		start = std::chrono::high_resolution_clock::now();
	}

	void stop()
	{
		auto finish = std::chrono::high_resolution_clock::now();
		size_t runningTime = 
			std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
		std::cout << title << ": " << double(runningTime) / 1000 << "s" << std::endl;
	}

private:
	std::string title;
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

#endif // CRAWLER_TIMER_HPP
