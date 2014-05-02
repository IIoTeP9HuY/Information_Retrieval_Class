#ifndef CONCURRENT_FREQUENCY_TABLE_HPP
#define CONCURRENT_FREQUENCY_TABLE_HPP

#include <mutex>
#include <unordered_map>

namespace fileindex
{

class ConcurrentFrequencyTable
{
public:
    ConcurrentFrequencyTable();

    void addWord(const std::string& word, size_t number = 1);

    int getWordFrequency(const std::string& word) const;

    std::unordered_map<std::string, int> getWordsFrequency() const;

private:
    std::unordered_map<std::string, int> wordsFrequency;
    mutable std::mutex tableMutex;
};

} // namespace fileindex

#endif // CONCURRENT_FREQUENCY_TABLE_HPP
