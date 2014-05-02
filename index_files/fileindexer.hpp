#ifndef FILEINDEXER_HPP
#define FILEINDEXER_HPP

#include <thread>

#include "concurrent_queue.hpp"
#include "concurrent_frequency_table.hpp"

namespace fileindex
{

class FileIndexer
{
public:
    FileIndexer(ConcurrentQueue<std::string>& filesForProcessingQueue,
                ConcurrentFrequencyTable& wordsFrequencyTable);

    ~FileIndexer();

    void start();

    void wait();

    void stop();

private:
    void run();

    void index(const std::string& path);

    std::unordered_map<std::string, int> localWordsFrequencyTable;
    ConcurrentQueue<std::string>& filesForProcessingQueue;
    ConcurrentFrequencyTable& wordsFrequencyTable;
    std::thread processingThread;
    size_t indexedFilesNumber;
    bool isWaitingForInput;
    bool isRunning;
};

} // namespace fileindex

#endif // FILEINDEXER_HPP
