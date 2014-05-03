#ifndef FILEFINDER_HPP
#define FILEFINDER_HPP

#include <thread>
#include <queue>
#include <unordered_set>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include "concurrent_queue.hpp"

using boost::regex;
using std::string;

namespace filecrawler
{

class FileFinder
{
public:
    explicit FileFinder(ConcurrentQueue<string>& filesForProcessingQueue,
                        regex fileFilterRegex = regex(".*"));

    ~FileFinder();

    void addPathForProcessing(const string& pathname);

    void start();

    void wait();

    void stop();

private:
    void run();

    void processPath(const string& pathname);

    std::unordered_set<std::string> visitedDirectories;
    ConcurrentQueue<string>& filesForProcessingQueue;
    regex fileFilterRegex;
    std::queue<string> pathsForProcessing;
    std::thread processingThread;
    size_t processedPathsNumber;
    size_t foundFilesNumber;

    bool isRunning;
};

} // namespace filecrawler

#endif // FILEFINDER_HPP
