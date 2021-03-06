#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <queue>
#include <limits>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>

#include "filecrawler/logger.hpp"
#include "filecrawler/fileprocessor.hpp"
#include "filecrawler/filefinder.hpp"

#include "webgraph_builder.hpp"

namespace po = boost::program_options;

using namespace webgraph;

std::string to_string(std::vector<std::string> arg)
{
    std::ostringstream oss;
    std::copy(arg.begin(), arg.end(), std::ostream_iterator<std::string>(oss, " "));
    std::string result = oss.str();
    result.erase(--result.end());
    return result;
}

void calculateInOutStatistics(const Webgraph &webgraph) {
    std::vector<int> inDegrees(webgraph.verticesNumber(), 0);
    std::vector<int> outDegrees(webgraph.verticesNumber(), 0);
    for (size_t i = 0; i < webgraph.verticesNumber(); ++i) {
        for (auto edge : webgraph.getLinks(i)) {
            ++outDegrees[i];
            ++inDegrees[edge.destination];
        }
    }

    std::ofstream ofs("in_out_stats");
    for (size_t i = 0; i < webgraph.verticesNumber(); ++i) {
        ofs << webgraph.getUrl(i) << " " << inDegrees[i] << " " << outDegrees[i] << '\n';
    }
    ofs.close();
}

std::vector<size_t> calculateDistances(Webgraph::Vertex source, const Webgraph &webgraph) {
    std::vector<size_t> distances(webgraph.verticesNumber(), webgraph.verticesNumber() + 1);
    distances[source] = 0;

    std::queue<Webgraph::Vertex> verticesQueue;
    verticesQueue.push(source);

    while (!verticesQueue.empty()) {
        Webgraph::Vertex currentVertex = verticesQueue.front();
        verticesQueue.pop();

        for (Webgraph::Link link : webgraph.getLinks(currentVertex)) {
            Webgraph::Vertex nextVertex = link.destination;
            if (distances[currentVertex] + 1 < distances[nextVertex]) {
                distances[nextVertex] = distances[currentVertex] + 1;
                verticesQueue.push(nextVertex);
            }
        }
    }

    return distances;
}

std::vector<double> calculatePageranks(const Webgraph &webgraph) {
    std::vector<double> pageranks[2];
    pageranks[0] = std::vector<double>(webgraph.verticesNumber(), 1.0 / webgraph.verticesNumber());

    const double DAMPING = 0.85;
    const size_t ITERATIONS = 30;
    for (size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        logging::Log::debug("Pagerank iteration: ", iteration);
        int current = (iteration) % 2;
        int next = (iteration + 1) % 2;
        pageranks[next] = std::vector<double>(webgraph.verticesNumber(), (1 - DAMPING) / webgraph.verticesNumber());
        for (Webgraph::Vertex source = 0; source < webgraph.verticesNumber(); ++source) {
            for (Webgraph::Link link : webgraph.getLinks(source)) {
                Webgraph::Vertex destination = link.destination;
                pageranks[next][destination] += DAMPING * (pageranks[current][source] / webgraph.getLinks(source).size());
            }
        }
    }

    return pageranks[ITERATIONS % 2];
}

void buildWebgraph(const std::string &path, const std::string &domain, size_t threadsNumber)
{
    logging::Log::info("Building webgraph from '", path, "' for domain '", domain, "' using ", threadsNumber, " threads");

    WebgraphBuilder webgraphBuilder(threadsNumber);
    Webgraph webgraph = webgraphBuilder.build(path, domain, boost::regex(".*\\.html"));

    logging::Log::info("Webraph sites: ", webgraph.verticesNumber());
    logging::Log::info("Webraph links: ", webgraph.edgesNumber());

    logging::Log::info("Calculating In-Out statistics");
    calculateInOutStatistics(webgraph);
    {
        logging::Log::info("Calculating distance");
        std::ofstream ofs("distances");
        auto distances = calculateDistances(webgraph.urlToIndex(NCrawler::addFileExtension(domain)), webgraph);
        for (size_t i = 0; i < webgraph.verticesNumber(); ++i) {
            ofs << webgraph.getUrl(i) << " " << distances[i] << '\n';
        }
        ofs.close();
    }
    {
        logging::Log::info("Calculating pagerank");
        std::ofstream ofs("pagerank");
        auto pageranks = calculatePageranks(webgraph);
        for (size_t i = 0; i < webgraph.verticesNumber(); ++i) {
            ofs << webgraph.getUrl(i) << " " << pageranks[i] << '\n';
        }
        ofs.close();
    }
}

int main(int argc, char *argv[]) {
    size_t threadsNumber;
    std::string path;
    std::string domain;
    po::options_description generic("Generic options");
    generic.add_options()
        ("help", "produce help message")
        ("threads,t", po::value<size_t>(&threadsNumber)->default_value(3), "set threads number")
        ("path", po::value<std::string>(&path), "set path with downloaded urls")
        ("domain", po::value<std::string>(&domain), "set domain url")
        ("verbose,v", "set verbose")
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
        std::cout << "Usage: " << argv[0] << " --path PATH --domain URL" << std::endl;
        std::cout << generic << std::endl;
        return 1;
    }

    if (threadsNumber == 0)
    {
        std::cerr << "Wrong number of threads" << std::endl;
    }

    if (!vm.count("path") || !vm.count("domain"))
    {
        std::cout << "Usage: " << argv[0] << " --path PATH --domain URL" << std::endl;
        std::cerr << "Try '" << argv[0] << " --help' for more information" << std::endl;
        return 1;
    }

    if (vm.count("verbose"))
    {
        logging::Log::info.setVerbose(true);
    }

    buildWebgraph(path, domain, threadsNumber);

	return 0;
}