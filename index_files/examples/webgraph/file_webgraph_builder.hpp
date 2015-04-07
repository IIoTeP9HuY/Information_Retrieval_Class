#ifndef FILE_WEBGRAPH_BUILDER_HPP
#define FILE_WEBGRAPH_BUILDER_HPP

#include <thread>

#include "crawler/url_utils.hpp"

#include "filecrawler/concurrent_queue.hpp"

#include "webgraph.hpp"

namespace webgraph {

using filecrawler::FileProcessor;
using filecrawler::ConcurrentQueue;

using NCrawler::URL;

class FileWebgraphBuilder : public FileProcessor {
public:
    FileWebgraphBuilder(ConcurrentQueue<std::string>& filesForProcessingQueue,
        Webgraph &webgraph, std::mutex &webgraphMutex, const std::string &domain):
    	FileProcessor(filesForProcessingQueue), webgraph(webgraph), 
        webgraphMutex(webgraphMutex), domain(domain) {
    }

    ~FileWebgraphBuilder() {
    }

private:
    void mergeThreadResources() {
        std::lock_guard<std::mutex> guard(webgraphMutex);
        for (const auto &edge : edges) {
            auto source = webgraph.addUrl(edge.first);
            auto destination = webgraph.addUrl(edge.second);
            webgraph.addLink(source, destination);
        }
        edges.clear();
    }

    bool process(const std::string& path) {
        Log::debug("Processing file ", path);

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

        std::string data;
        data.resize(fileSizeInBytes);
        infile.seekg(0, std::ios::beg);
        infile.read(&data[0], fileSizeInBytes);

        URL domainURL = domain;

        auto pos = path.find(domainURL);
        URL fileUrl(path.begin() + pos, path.end());
        auto urls = NCrawler::getUrls(domainURL, data);
        for (auto &url : urls) {
            if (NCrawler::isAllowed(domainURL, url)) {
                url = NCrawler::addFileExtension(url);
                edges.push_back(std::make_pair(fileUrl, url));
            }
        }

        return true;
    }

    Webgraph &webgraph;
    std::mutex &webgraphMutex;
    const std::string &domain;

    std::vector< std::pair<std::string, std::string> > edges;
};

} // namespace webgraph

#endif // FILE_WEBGRAPH_BUILDER_HPP
