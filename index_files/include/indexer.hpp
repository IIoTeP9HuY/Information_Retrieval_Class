#ifndef INDEXER_HPP
#define INDEXER_HPP

#include <vector>
#include <unordered_map>
#include <boost/regex.hpp>

namespace fileindex
{

class Indexer
{
public:
    Indexer(size_t threadsNumber = 2);

    ~Indexer();

    std::unordered_map<std::string, int> indexPaths(const std::vector<std::string>& paths,
                                                    boost::regex fileFilterRegex);

private:
    size_t threadsNumber;

};

} // namespace fileindex

#endif // INDEXER_HPP
