#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
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

namespace po = boost::program_options;

using namespace simhash;

std::string to_string(std::vector<std::string> arg)
{
    std::ostringstream oss;
    std::copy(arg.begin(), arg.end(), std::ostream_iterator<std::string>(oss, " "));
    std::string result = oss.str();
    result.erase(--result.end());
    return result;
}

std::vector<DocumentSimilarityInfo> buildSimhashes(const std::string &path, size_t threadsNumber)
{
    logging::Log::info("Building simhashes from '", path, "' using ", threadsNumber, " threads");

    SimhashBuilder simhashBuilder(threadsNumber);
    std::vector<DocumentSimilarityInfo> documentInfos = simhashBuilder.build(path, boost::regex(".*\\.html"));
    logging::Log::info("Documents infos number: ", documentInfos.size());

    return documentInfos;
}

size_t simhashDistance(const Simhash &lhs, const Simhash &rhs) {
    if (lhs.length() != rhs.length()) {
        throw std::invalid_argument("Different lengths of passed simhashes");
    }

    size_t distance = 0;
    for (size_t i = 0; i < lhs.length(); ++i) {
        distance += lhs[i] != rhs[i];
    }
    return distance;
}

std::vector<std::vector<size_t>> findSimilar(std::vector<DocumentSimilarityInfo> documentInfos, size_t simhashBitsDistance) {
    std::vector<std::vector<size_t>> similarDocuments(documentInfos.size());
    for (size_t i = 0; i < documentInfos.size(); ++i) {
        for (size_t j = i + 1; j < documentInfos.size(); ++j) {
            size_t s1 = documentInfos[i].size;
            size_t s2 = documentInfos[j].size;
            double proportion = abs(s1 - s2) * 1.0 / (s1 + s2);
            if (proportion > (0.25) / (0.25 + 2)) {
                continue;
            }
            size_t distance = simhashDistance(documentInfos[i].simhash, documentInfos[j].simhash);
            if (distance < simhashBitsDistance) {
                similarDocuments[i].push_back(j);
                similarDocuments[j].push_back(i);
            }
        }
    }
    return similarDocuments;
}

std::vector<std::vector<size_t>> findClusters(const std::vector<std::vector<size_t>> &similarDocuments) {
    std::vector<std::vector<size_t>> clusters;
    std::set<std::pair<size_t, size_t>, std::greater<std::pair<int, int>>> verticesPowers;
    std::vector<char> clustered(similarDocuments.size(), false);
    std::vector<size_t> currentVertexPower(similarDocuments.size(), 0);

    for (size_t i = 0; i < similarDocuments.size(); ++i) {
        verticesPowers.insert(std::make_pair(similarDocuments[i].size(), i));
        currentVertexPower[i] = similarDocuments[i].size();
    }

    while (!verticesPowers.empty()) {
        auto vertexPair = *verticesPowers.begin();
        verticesPowers.erase(verticesPowers.begin());

        size_t document = vertexPair.second;
        if (clustered[document]) {
            continue;
        }
        clustered[document] = true;
        clusters.push_back(std::vector<size_t>());
        clusters.back().push_back(document);
        for (size_t i = 0; i < similarDocuments[document].size(); ++i) {
            size_t similarDocument = similarDocuments[document][i];
            if (clustered[similarDocument]) {
                continue;
            }
            clustered[similarDocument] = true;

            for (size_t j = 0; j < similarDocuments[similarDocument].size(); ++j) {
                size_t similarToSimilarDocument = similarDocuments[similarDocument][j];
                if (clustered[similarToSimilarDocument]) {
                    continue;
                }
                size_t &vertexPower = currentVertexPower[similarToSimilarDocument];
                verticesPowers.erase(std::make_pair(vertexPower, similarToSimilarDocument));
                --vertexPower;
                verticesPowers.insert(std::make_pair(vertexPower, similarToSimilarDocument));
            }

            clusters.back().push_back(similarDocument);
        }
    }

    return clusters;
}

int main(int argc, char *argv[]) {
    size_t threadsNumber;
    size_t simhashBitsDistance;
    std::string path;
    po::options_description generic("Generic options");
    generic.add_options()
        ("help", "produce help message")
        ("threads,t", po::value<size_t>(&threadsNumber)->default_value(3), "set threads number")
        ("path", po::value<std::string>(&path), "set path with downloaded urls")
        ("verbose,v", "set verbose")
        ("build,b", "set build mode")
        ("find,f", "set find mode")
        ("bits,s", po::value<size_t>(&simhashBitsDistance)->default_value(5), "set simhash bits distance")
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

    std::vector<DocumentSimilarityInfo> documentInfos;
    if (vm.count("build")) {
	    if (!vm.count("path"))
	    {
	        std::cout << "Usage: " << argv[0] << " --build --path PATH" << std::endl;
	        std::cerr << "Try '" << argv[0] << " --help' for more information" << std::endl;
	        return 1;
	    }

	    documentInfos = buildSimhashes(path, threadsNumber);

        std::ofstream ofs("simhashes");
    	for (const auto &documentInfo : documentInfos) {
    		ofs << documentInfo.path << " " << documentInfo.size << " " << documentInfo.simhash << '\n';
        }
        ofs.close();
	} else {
        std::ifstream ifs("simhashes");
        while (!ifs.eof()) {
        	std::string path, simhash;
            size_t size;
        	ifs >> path >> size >> simhash;
        	if (ifs.eof()) {
        		break;
        	}
        	documentInfos.push_back(DocumentSimilarityInfo(path, simhash, size));
        }
        ifs.close();
	}

	if (vm.count("find"))
    {
    	auto similarDocuments = findSimilar(documentInfos, simhashBitsDistance);
        auto clusters = findClusters(similarDocuments);
        // for (size_t i = 0; i < similarDocuments.size(); ++i) {
        //     for (size_t j = 0; j < similarDocuments[i].size(); ++j) {
        //         size_t d1 = i;
        //         size_t d2 = similarDocuments[i][j];
        //         Log::info("Similar: ", documentInfos[d1].path, " ", documentInfos[d2].path);
        //     }
        // }
        {
            std::ofstream ofs("clusters");
            for (size_t i = 0; i < clusters.size(); ++i) {
                ofs << "Cluster number: " << i << '\n';
                for (size_t j = 0; j < clusters[i].size(); ++j) {
                    size_t document = clusters[i][j];
                    ofs << documentInfos[document].path << '\n';
                }
            }
            ofs.close();
        }
    }
	return 0;
}