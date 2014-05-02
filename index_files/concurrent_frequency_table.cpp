#include "concurrent_frequency_table.hpp"

namespace fileindex
{

ConcurrentFrequencyTable::ConcurrentFrequencyTable()
{
}

void ConcurrentFrequencyTable::addWord(const std::string& word, size_t number)
{
    std::lock_guard<std::mutex> lock(tableMutex);
    wordsFrequency[word] += number;
}

int ConcurrentFrequencyTable::getWordFrequency(const std::string& word) const
{
    std::lock_guard<std::mutex> lock(tableMutex);
    return wordsFrequency.at(word);
}

std::unordered_map<std::string, int> ConcurrentFrequencyTable::getWordsFrequency() const
{
    std::lock_guard<std::mutex> lock(tableMutex);
    return wordsFrequency;
}

} // namespace fileindex
