#include "index_files/filefinder.hpp"

#include <vector>
#include <boost/filesystem.hpp>

#include "index_files/logger.hpp"

using namespace logging;

namespace fileindex
{

FileFinder::FileFinder(ConcurrentQueue<string>& filesForProcessingQueue,
                       boost::regex fileFilterRegex):
    filesForProcessingQueue(filesForProcessingQueue), fileFilterRegex(fileFilterRegex),
    processedPathsNumber(0), foundFilesNumber(0), isRunning(false)
{
}

FileFinder::~FileFinder()
{
}

void FileFinder::addPathForProcessing(const string& pathname)
{
    pathsForProcessing.push(pathname);
}

void FileFinder::start()
{
    Log::debug("Starting FileFinder");

    isRunning = true;
    processingThread = std::thread(&FileFinder::run, this);
}

void FileFinder::wait()
{
    Log::debug("Waiting for FileFinder");

    if (isRunning)
    {
        processingThread.join();
        isRunning = false;
    }

    Log::info("Processed paths number: ", processedPathsNumber);
    Log::info("Found files number: ", foundFilesNumber);
}

void FileFinder::stop()
{
    Log::debug("Stopping FileFinder");

    isRunning = false;
}

void FileFinder::run()
{
    while (isRunning && !pathsForProcessing.empty())
    {
        std::string pathname = pathsForProcessing.front();
        pathsForProcessing.pop();
        boost::filesystem::path path(pathname);
        if (boost::filesystem::exists(path))
        {
            path = boost::filesystem::complete(path);
            path = path.normalize();
            std::string absolutePath = path.string();

            if (visitedDirectories.find(absolutePath) == visitedDirectories.end())
            {
                visitedDirectories.insert(absolutePath);
                processPath(pathname);
                ++processedPathsNumber;
            }
        }
        else
        {
            Log::warn("Path ", pathname, " does not exists");
        }
    }
}

void FileFinder::processPath(const string& pathname)
{
    boost::filesystem::path path(pathname);
    try
    {
        if (boost::filesystem::exists(path))
        {
            if (boost::filesystem::is_directory(path))
            {
                Log::debug("Processing directory ", path);

                std::vector<boost::filesystem::path> children;
                copy(boost::filesystem::directory_iterator(path),
                     boost::filesystem::directory_iterator(), std::back_inserter(children));

                for (size_t i = 0; i < children.size(); ++i)
                {
                    const boost::filesystem::path child = children[i];
                    if (boost::filesystem::is_directory(child))
                    {
                        Log::debug("Adding directory: ", child.string(), " to search space");
                        pathsForProcessing.push(child.string());
                    }

                    if (boost::filesystem::is_regular_file(child))
                    {
                        boost::smatch matches;
                        if (boost::regex_match(child.string(), matches, fileFilterRegex))
                        {
                            Log::debug("Found matching file ", child.string());

                            ++foundFilesNumber;
                            filesForProcessingQueue.push(child.string());
                        }
                    }
                }
            }
            else
            {
                Log::warn(path, " is not a directory");
            }
        }
        else
        {
            Log::warn("Path ", path, " does not exists");
        }
    }
    catch (const boost::filesystem::filesystem_error& ex)
    {
        Log::warn(ex.what());
    }
}

} // namespace fileindex
