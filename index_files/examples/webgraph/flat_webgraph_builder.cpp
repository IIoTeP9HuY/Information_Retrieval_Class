#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <queue>
#include <algorithm>
#include <limits>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>

#include "filecrawler/logger.hpp"
#include "filecrawler/fileprocessor.hpp"
#include "filecrawler/filefinder.hpp"

#include "webgraph_builder.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

typedef std::string URL;

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

void processFile(const std::string& path, const std::string& domain, const URL& sourceURL, Webgraph& webgraph) {
    Log::debug("Processing file ", path);

    std::ifstream infile;
    infile.open(path, std::ios::binary);

    if (!infile.is_open())
    {
        Log::warn("Failed to open file ", path);
        return;
    }

    infile.seekg(0, std::ios::end);
    size_t fileSizeInBytes = infile.tellg();

    Log::debug("File size in bytes: ", fileSizeInBytes);

    std::string data;
    data.resize(fileSizeInBytes);
    infile.seekg(0, std::ios::beg);
    infile.read(&data[0], fileSizeInBytes);

    URL domainURL = domain;

    auto urls = NCrawler::getUrls(domainURL, data);
    auto source = webgraph.addUrl(sourceURL);
    urls.erase(std::unique(urls.begin(), urls.end()), urls.end());
    for (auto &url : urls) {
        if (NCrawler::isAllowed(domainURL, url)) {
            auto destination = webgraph.addUrl(url);
            webgraph.addLink(source, destination);
        }
    }
}

void buildWebgraph(const std::string &path,
    const std::string& domain,
    const std::string& urlMappingPath,
    const std::string& startPage)
{
    logging::Log::info("Building webgraph from '", path, "' for domain '", domain);

    std::ifstream urlMappingStream(urlMappingPath);
    if (!urlMappingStream.is_open()) {
        logging::Log::error("Failed to open url mapping file ", urlMappingPath);
        return;
    }

    Webgraph webgraph;
    std::string filename;
    std::string url;
    int urlsProcessed = 0;
    webgraph.addUrl(startPage);
    while (urlMappingStream >> filename >> url) {
        fs::path Path(path);
        fs::path Filename(filename);
        fs::path FilePath = Path / Filename;
        processFile(FilePath.string(), domain, url, webgraph);
        ++urlsProcessed;
        if (urlsProcessed % 10000 == 0) {
            logging::Log::warn("Urls processed: ", urlsProcessed);
        }
    }

    logging::Log::info("Webraph sites: ", webgraph.verticesNumber());
    logging::Log::info("Webraph links: ", webgraph.edgesNumber());

    logging::Log::info("Calculating In-Out statistics");
    calculateInOutStatistics(webgraph);
    {
        logging::Log::info("Calculating distance");
        std::ofstream ofs("distances");
        auto distances = calculateDistances(webgraph.urlToIndex(startPage), webgraph);
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
    std::string path;
    std::string domain;
    std::string startPage;
    std::string urlMapping;
    po::options_description generic("Generic options");
    generic.add_options()
        ("help", "produce help message")
        ("path", po::value<std::string>(&path), "set path with downloaded urls")
        ("domain", po::value<std::string>(&domain), "set domain url")
        ("start_page", po::value<std::string>(&startPage), "set start page")
        ("urlMapping", po::value<std::string>(&urlMapping), "set url mapping file")
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
        std::cout << "Usage: " << argv[0] << " --path PATH --domain URL --urlMapping PATH" << std::endl;
        std::cout << generic << std::endl;
        return 1;
    }

    if (!vm.count("path") || !vm.count("domain") || !vm.count("urlMapping"))
    {
        std::cout << "Usage: " << argv[0] << " --path PATH --domain URL --urlMapping PATH" << std::endl;
        std::cerr << "Try '" << argv[0] << " --help' for more information" << std::endl;
        return 1;
    }

    if (vm.count("verbose"))
    {
        logging::Log::info.setVerbose(true);
    }

    buildWebgraph(path, domain, urlMapping, startPage);

    return 0;
}
