#ifndef WEBGRAPH_BUILDER_HPP
#define WEBGRAPH_BUILDER_HPP

#include <thread>

#include "filecrawler/concurrent_queue.hpp"
#include "filecrawler/filefinder.hpp"

#include "file_webgraph_builder.hpp"
#include "webgraph.hpp"

namespace webgraph {

using filecrawler::FileFinder;

class WebgraphBuilder {
public:
	WebgraphBuilder(size_t threadsNumber): threadsNumber(threadsNumber) {}

    Webgraph build(const std::string& path, const std::string &domain, boost::regex fileFilterRegex) {
	    Webgraph webgraph;
	    std::mutex webgraphMutex;
	    ConcurrentQueue<std::string> filesForProcessingQueue;
	    FileFinder fileFinder(filesForProcessingQueue, fileFilterRegex);
        fileFinder.addPathForProcessing(path);
	    fileFinder.start();

	    std::vector<std::shared_ptr<FileWebgraphBuilder>> fileWebgraphBuilders;
	    for (size_t i = 0; i < threadsNumber; ++i)
	    {
	        fileWebgraphBuilders.emplace_back(
	        	new FileWebgraphBuilder(filesForProcessingQueue, webgraph, webgraphMutex, domain)
	        );
	    }

	    for (size_t i = 0; i < threadsNumber; ++i)
	    {
	        fileWebgraphBuilders[i]->start();
	    }

	    fileFinder.wait();
	    for (size_t i = 0; i < threadsNumber; ++i)
	    {
	        fileWebgraphBuilders[i]->wait();
	    }

	    return std::move(webgraph);
    }

private:
    size_t threadsNumber;
};

} // namespace webgraph

#endif // WEBGRAPH_BUILDER_HPP