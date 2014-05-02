#include "fileindexer.hpp"

#include <fstream>
#include <cctype>
#include <array>
#include <boost/optional.hpp>

using namespace logging;

namespace fileindex
{

FileIndexer::FileIndexer(ConcurrentQueue<std::string>& filesForProcessingQueue,
                         ConcurrentFrequencyTable& wordsFrequencyTable):
    filesForProcessingQueue(filesForProcessingQueue),
    wordsFrequencyTable(wordsFrequencyTable), indexedFilesNumber(0),
    isWaitingForInput(false), isRunning(false)
{
}

FileIndexer::~FileIndexer()
{
}

void FileIndexer::start()
{
    Log::debug("Starting FileIndexer");

    isRunning = true;
    isWaitingForInput = true;
    processingThread = std::thread(&FileIndexer::run, this);
}

void FileIndexer::wait()
{
    Log::debug("Waiting for FileIndexer");

    if (isRunning)
    {
        isWaitingForInput = false;
        processingThread.join();
        isRunning = false;
    }
    Log::info("Indexed files number: ", indexedFilesNumber);
}

void FileIndexer::stop()
{
    Log::debug("Stopping FileIndexer");

    isWaitingForInput = false;
    isRunning = false;
}

void FileIndexer::run()
{
    while (isRunning && (isWaitingForInput || !filesForProcessingQueue.empty()))
    {
        boost::optional<std::string> path = filesForProcessingQueue.pull();
        if (path)
        {
            index(path.get());
        }
        else
        {
            Log::debug("Queue is empty, nothing to get");
        }
    }

    for (auto it = localWordsFrequencyTable.begin(); it != localWordsFrequencyTable.end(); ++it)
    {
        wordsFrequencyTable.addWord(it->first, it->second);
    }
}

void FileIndexer::index(const std::string& path)
{
    Log::debug("Indexing file ", path);

    std::ifstream infile;
    infile.open(path, std::ios::binary);

    if (!infile.is_open())
    {
        Log::warn("Failed to open file ", path);
        return;
    }

    infile.seekg(0, std::ios::end);
    size_t fileSizeInBytes = infile.tellg();

    Log::debug("File size in bytes: ", fileSizeInBytes);

    std::vector<char> data;
    data.resize(fileSizeInBytes);
    infile.seekg(0, std::ios::beg);
    infile.read(&data[0], fileSizeInBytes);

    std::string currentWord;
    for (size_t i = 0; i <= data.size(); ++i)
    {
        if (i != data.size() && isalnum(data[i]))
        {
            currentWord += tolower(data[i]);
        }
        else
        {
            if (!currentWord.empty())
            {
                ++localWordsFrequencyTable[currentWord];
                currentWord.clear();
            }
        }
    }
    ++indexedFilesNumber;
}

} // namespace fileindex
