#ifndef FILEINDEXER_HPP
#define FILEINDEXER_HPP

#include <thread>

#include "filecrawler/fileprocessor.hpp"

#include "concurrent_frequency_table.hpp"

namespace fileindex
{

using filecrawler::FileProcessor;
using filecrawler::ConcurrentQueue;

class FileIndexer : public FileProcessor
{
public:
    FileIndexer(ConcurrentQueue<std::string>& filesForProcessingQueue,
                ConcurrentFrequencyTable& wordsFrequencyTable);

    ~FileIndexer();

private:
    void mergeThreadResources();

    bool process(const std::string& path);

    std::unordered_map<std::string, int> localWordsFrequencyTable;
    ConcurrentFrequencyTable& wordsFrequencyTable;
};

} // namespace fileindex

#endif // FILEINDEXER_HPP
