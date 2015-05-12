#ifndef SIMHASH_BUILDER_HPP
#define SIMHASH_BUILDER_HPP

#include <thread>

#include "filecrawler/concurrent_queue.hpp"
#include "filecrawler/filefinder.hpp"

#include "file_simhash_builder.hpp"

namespace simhash {

using filecrawler::FileFinder;

class SimhashBuilder {
public:
    SimhashBuilder(size_t threadsNumber): threadsNumber(threadsNumber) {}

    std::vector<DocumentSimilarityInfo> build(const std::string& path, boost::regex fileFilterRegex) {
        std::vector<DocumentSimilarityInfo> documentInfos;
        std::mutex documentInfosMutex;
        ConcurrentQueue<std::string> filesForProcessingQueue;
        FileFinder fileFinder(filesForProcessingQueue, fileFilterRegex);
        fileFinder.addPathForProcessing(path);
        fileFinder.start();

        std::vector<std::shared_ptr<FileSimhashBuilder>> fileSimhashBuilders;
        for (size_t i = 0; i < threadsNumber; ++i)
        {
            fileSimhashBuilders.emplace_back(
                new FileSimhashBuilder(filesForProcessingQueue, documentInfos, documentInfosMutex)
            );
        }

        for (size_t i = 0; i < threadsNumber; ++i)
        {
            fileSimhashBuilders[i]->start();
        }

        fileFinder.wait();
        for (size_t i = 0; i < threadsNumber; ++i)
        {
            fileSimhashBuilders[i]->wait();
        }

        return std::move(documentInfos);
    }

private:
    size_t threadsNumber;
};

} // namespace simhash

#endif // SIMHASH_BUILDER_HPP
