#include "indexer.hpp"

#include "filecrawler/filefinder.hpp"

#include "fileindexer.hpp"

namespace fileindex
{

using filecrawler::FileFinder;

Indexer::Indexer(size_t threadsNumber): threadsNumber(threadsNumber)
{
}

Indexer::~Indexer()
{
}

std::unordered_map<std::string, int> Indexer::indexPaths(const std::vector<std::string>& paths,
                                                         boost::regex fileFilterRegex)
{
    ConcurrentFrequencyTable frequencyTable;
    ConcurrentQueue<std::string> filesForProcessingQueue;
    FileFinder fileFinder(filesForProcessingQueue, fileFilterRegex);
    for (size_t i = 0; i < paths.size(); ++i)
    {
        fileFinder.addPathForProcessing(paths[i]);
    }
    fileFinder.start();

    std::vector<std::shared_ptr<FileIndexer>> fileIndexers;
    for (size_t i = 0; i < threadsNumber; ++i)
    {
        FileIndexer* fileIndexer = new FileIndexer(filesForProcessingQueue, frequencyTable);
        fileIndexers.push_back(std::shared_ptr<FileIndexer> (fileIndexer));
    }

    for (size_t i = 0; i < threadsNumber; ++i)
    {
        fileIndexers[i]->start();
    }

    fileFinder.wait();
    for (size_t i = 0; i < threadsNumber; ++i)
    {
        fileIndexers[i]->wait();
    }

    return frequencyTable.getWordsFrequency();
}

} // namespace fileindex
