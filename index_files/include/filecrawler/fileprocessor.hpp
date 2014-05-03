#ifndef FILEPROCESSOR_HPP
#define FILEPROCESSOR_HPP

#include <thread>

#include "concurrent_queue.hpp"

namespace filecrawler
{

class FileProcessor
{
public:
    FileProcessor(ConcurrentQueue<std::string>& filesForProcessingQueue);

    virtual ~FileProcessor();

    void start();

    void wait();

    void stop();

protected:
    void run();

    virtual void mergeThreadResources() = 0;

    virtual bool process(const std::string& path) = 0;

    ConcurrentQueue<std::string>& filesForProcessingQueue;
    std::thread processingThread;
    size_t processedFilesNumber;
    bool isWaitingForInput;
    bool isRunning;
};

} // namespace filecrawler

#endif // FILEPROCESSOR_HPP
