#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>

#include "filecrawler/logger.hpp"

#include "indexer.hpp"

const size_t topNumber = 10;

namespace po = boost::program_options;

std::string to_string(std::vector<std::string> arg)
{
    std::ostringstream oss;
    std::copy(arg.begin(), arg.end(), std::ostream_iterator<std::string>(oss, " "));
    std::string result = oss.str();
    result.erase(--result.end());
    return result;
}

struct greaterSecond {
    template<typename T1, typename T2>
    inline bool operator()(const std::pair<T1, T2>& lhs, const std::pair<T1, T2>& rhs)
    {
        return lhs.second > rhs.second;
    }
};

struct plus2nd {
    template <typename T1, typename T2>
    inline T1 operator()(T1 lhs, const T2 &rhs) const
    {
        return lhs + rhs.second;
    }
};

void runIndexing(std::vector<std::string> paths, size_t threadsNumber)
{
    logging::Log::info("Starting indexing in ", to_string(paths), " with ", threadsNumber, " threads");

    fileindex::Indexer indexer(threadsNumber);
    std::unordered_map<std::string, int> wordsFrequencyTable =
            indexer.indexPaths(paths, boost::regex(".*\\.hpp"));

    typedef std::pair<std::string, int> WordFrequency;
    std::vector<WordFrequency> wordsFrequency(wordsFrequencyTable.begin(),
                                              wordsFrequencyTable.end());

    logging::Log::info("Different words number: ", wordsFrequency.size());

//    size_t totalWordsNumber = std::accumulate(wordsFrequency.begin(), wordsFrequency.end(), 0,
//                    [](size_t init, const WordFrequency& wf) { return init + wf.second; });

    size_t totalWordsNumber = std::accumulate(wordsFrequency.begin(), wordsFrequency.end(), 0,
                                              plus2nd());

    logging::Log::info("Total words number: ", totalWordsNumber);

//    std::sort(wordsFrequency.begin(), wordsFrequency.end(),
//              [](WordFrequency lhs, WordFrequency rhs) { return lhs.second > rhs.second; });

    std::sort(wordsFrequency.begin(), wordsFrequency.end(), greaterSecond());

    for (size_t i = 0; i < topNumber && i < wordsFrequency.size(); ++i)
    {
        std::cout << wordsFrequency[i].first << " : " << wordsFrequency[i].second << std::endl;
    }
}

int main(int argc, char* argv[])
{
    size_t threadsNumber;
    std::vector<std::string> paths;
    po::options_description generic("Generic options");
    generic.add_options()
        ("help", "produce help message")
        ("threads,t", po::value<size_t>(&threadsNumber)->default_value(3), "set threads number")
        ("verbose,v", "set verbose")
    ;

    po::positional_options_description p;
    p.add("path", -1);

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("path", po::value<std::vector<std::string>>(&paths), "input path")
    ;

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(hidden);

    po::variables_map vm;

    try
    {
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    }
    catch (po::error& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << "Usage: " << argv[0] << " PATH" << std::endl;
        std::cout << generic << std::endl;
        return 1;
    }

    if (threadsNumber == 0)
    {
        std::cerr << "Wrong number of threads" << std::endl;
    }

    if (!vm.count("path"))
    {
        std::cout << "Usage: " << argv[0] << " PATH" << std::endl;
        std::cerr << "Try '" << argv[0] << " --help' for more information" << std::endl;
        return 1;
    }

    if (vm.count("verbose"))
    {
        logging::Log::info.setVerbose(true);
    }

    runIndexing(paths, threadsNumber);

    return 0;
}
