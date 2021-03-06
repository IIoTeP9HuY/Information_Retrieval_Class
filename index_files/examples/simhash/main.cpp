#include <iostream>
#include <fstream>
#include <numeric>
#include <fstream>
#include <queue>
#include <limits>
#include <set>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>

#include "filecrawler/logger.hpp"
#include "filecrawler/fileprocessor.hpp"
#include "filecrawler/filefinder.hpp"

#include "simhash_builder.hpp"
#include "clusters_builder.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using namespace simhash;

std::string to_string(std::vector<std::string> arg)
{
    std::ostringstream oss;
    std::copy(arg.begin(), arg.end(), std::ostream_iterator<std::string>(oss, " "));
    std::string result = oss.str();
    result.erase(--result.end());
    return result;
}

std::vector<DocumentInfo> buildSimhashes(const std::string &path, size_t threadsNumber)
{
    logging::Log::info("Building simhashes from '", path, "' using ", threadsNumber, " threads");

    SimhashBuilder simhashBuilder(threadsNumber);
    std::vector<DocumentSimilarityInfo> documentSimilarities = simhashBuilder.build(path, boost::regex(".*\\.txt"));
    logging::Log::info("Documents infos number: ", documentSimilarities.size());

    std::vector<DocumentInfo> documentInfos;
    for (size_t i = 0; i < documentSimilarities.size(); ++i) {
        documentInfos.push_back(DocumentInfo(i, std::move(documentSimilarities[i])));
    }

    return documentInfos;
}

bool vectorSizeComparator(const std::vector<size_t> &lhs, const std::vector<size_t> &rhs) {
    return lhs.size() > rhs.size();
}

int main(int argc, char *argv[]) {
    size_t threadsNumber;
    size_t simhashBitsDistance;
    std::string path;
    std::string urlsMapping;
    std::string reportPath;
    po::options_description generic("Generic options");
    generic.add_options()
        ("help", "produce help message")
        ("threads,t", po::value<size_t>(&threadsNumber)->default_value(3), "set threads number")
        ("path", po::value<std::string>(&path), "set path with downloaded urls")
        ("dest", po::value<std::string>(&reportPath)->default_value("."), "path to save reports")
        ("verbose,v", "set verbose")
        ("build,b", "set build mode")
        ("find,f", "set find mode")
        ("bits,s", po::value<size_t>(&simhashBitsDistance)->default_value(5), "set simhash bits distance")
        ("urlsMapping", po::value<std::string>(&urlsMapping)->default_value("urls"), "set path to urls mapping file")
        ;

    po::options_description cmdline_options;
    cmdline_options.add(generic);

    po::variables_map vm;

    try
    {
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
    }
    catch (po::error& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << "Usage: " << argv[0] << std::endl;
        std::cout << generic << std::endl;
        return 1;
    }

    if (threadsNumber == 0)
    {
        std::cerr << "Wrong number of threads" << std::endl;
    }

    if (vm.count("verbose"))
    {
        logging::Log::info.setVerbose(true);
    }

    std::vector<DocumentInfo> documentInfos;
    if (vm.count("build")) {
        if (!vm.count("path"))
        {
            std::cout << "Usage: " << argv[0] << " --build --path PATH" << std::endl;
            std::cerr << "Try '" << argv[0] << " --help' for more information" << std::endl;
            return 1;
        }

        std::ifstream urlsMappingStream(urlsMapping);
        if (!urlsMappingStream.is_open()) {
            logging::Log::error("Failed to open file with url mappings");
            return 0;
        }

        std::unordered_map<std::string, std::string> pathToUrl;

    	while (path.back() == '/') {
    		path.resize(path.length() - 1);
        }

        {
            std::string filename;
            std::string url;
            int urlsProcessed = 0;
            while (urlsMappingStream >> filename >> url) {
                auto templateName = std::to_string(urlsProcessed + 1) + ".txt";
                pathToUrl[path + "/" + templateName] = url;
                ++urlsProcessed;
            }
        }

        documentInfos = buildSimhashes(path, threadsNumber);

        std::ofstream ofs(reportPath + "/" + "simhashes");
        for (const auto &documentInfo : documentInfos) {
            if (pathToUrl.find(documentInfo.path) == pathToUrl.end()) {
                logging::Log::error("Unrecognized path: ", documentInfo.path);
                return 0;
            }
            std::string url = pathToUrl.at(documentInfo.path);
            ofs << url << " "
                << documentInfo.size << " "
                << documentInfo.simhash << '\n';
        }
        ofs.close();
    } else {
        std::ifstream ifs(reportPath + "/" + "simhashes");
        if (!ifs.is_open()) {
            logging::Log::error("Failed to open file with simhashes");
            return 0;
        }
        size_t id = 0;
        while (!ifs.eof()) {
            std::string path;
            Simhash simhash;
            size_t size;
            ifs >> path >> size >> simhash;
            if (ifs.eof()) {
                break;
            }
            documentInfos.push_back(
                DocumentInfo(id, DocumentSimilarityInfo(path, simhash, size)));
            ++id;
        }
        ifs.close();
    }

    std::unordered_map<size_t, std::string> idToPath;
    for (const auto &documentInfo : documentInfos) {
        idToPath[documentInfo.id] = documentInfo.path;
    }

    if (vm.count("find"))
    {
        ClustersBuilder clustersBuilder(simhashBitsDistance);
        auto clusters = clustersBuilder.build(documentInfos);

        sort(clusters.begin(), clusters.end(), &vectorSizeComparator);
        {
            std::ofstream ofs(
                reportPath + "/" + "clusters_" + std::to_string(simhashBitsDistance));
            for (size_t i = 0; i < clusters.size(); ++i) {
                ofs << "Cluster number: " << i << '\n';
                for (size_t j = 0; j < clusters[i].size(); ++j) {
                    size_t document = clusters[i][j];
                    ofs << idToPath[document] << '\n';
                }
            }
            ofs.close();
        }

        {
            std::ofstream ofs(
                reportPath + "/" + "clusters_" + std::to_string(simhashBitsDistance) + "_sizes");
            for (size_t i = 0; i < clusters.size(); ++i) {
                ofs << i << " " << clusters[i].size() << '\n';
            }
            ofs.close();
        }
    }
    return 0;
}
