#include "index_files/fileprocessor.hpp"

#include <fstream>
#include <cctype>
#include <array>
#include <boost/optional.hpp>

#include "index_files/logger.hpp"

using namespace logging;

namespace fileindex
{

FileProcessor::FileProcessor(ConcurrentQueue<std::string>& filesForProcessingQueue):
	filesForProcessingQueue(filesForProcessingQueue), processedFilesNumber(0), 
	isWaitingForInput(false), isRunning(false)
{
}

FileProcessor::~FileProcessor()
{
}

void FileProcessor::start()
{
    Log::debug("Starting FileProcessor");

    isRunning = true;
    isWaitingForInput = true;
    processingThread = std::thread(&FileProcessor::run, this);
}

void FileProcessor::wait()
{
    Log::debug("Waiting for FileProcessor");

    if (isRunning)
    {
        isWaitingForInput = false;
        processingThread.join();
        isRunning = false;
    }
    Log::info("Processed files number: ", processedFilesNumber);
}

void FileProcessor::stop()
{
    Log::debug("Stopping FileProcessor");

    isWaitingForInput = false;
    isRunning = false;
}

void FileProcessor::run()
{
    while (isRunning && (isWaitingForInput || !filesForProcessingQueue.empty()))
    {
        boost::optional<std::string> path = filesForProcessingQueue.pull();
        if (path)
        {
            bool processed = process(path.get());
            if (processed) {
            	++processedFilesNumber;
            }
        }
        else
        {
            Log::debug("Queue is empty, nothing to get");
        }
    }

    mergeThreadResources();
}

} // namespace fileindex
