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
    FileProcessor(filesForProcessingQueue), wordsFrequencyTable(wordsFrequencyTable)
{
}

FileIndexer::~FileIndexer()
{
}

void FileIndexer::mergeThreadResources()
{
    for (auto it = localWordsFrequencyTable.begin(); it != localWordsFrequencyTable.end(); ++it)
    {
        wordsFrequencyTable.addWord(it->first, it->second);
    }
}

bool FileIndexer::process(const std::string& path)
{
    Log::debug("Indexing file ", path);

    std::ifstream infile;
    infile.open(path, std::ios::binary);

    if (!infile.is_open())
    {
        Log::warn("Failed to open file ", path);
        return false;
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
    return true;
}

} // namespace fileindex
