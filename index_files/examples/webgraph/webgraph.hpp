#ifndef WEBGRAPH_HPP
#define WEBGRAPH_HPP

#include <thread>
#include <unordered_map>
#include <vector>

namespace webgraph {

class Webgraph {
public:
	typedef size_t Vertex;

	struct Link {
		Link(Vertex destination): destination(destination) {}

		Vertex destination;
	};

	Webgraph(): m_edgesNumber(0) {}

	size_t verticesNumber() const {
		return m_links.size();
	}

	size_t edgesNumber() const {
		return m_edgesNumber;
	}

	bool containsUrl(const std::string &url) const {
		return m_urlToIndex.find(url) != m_urlToIndex.end();
	}

	Vertex addUrl(const std::string &url) {
		if (containsUrl(url)) {
			return m_urlToIndex[url];
		}

		Vertex index = newVertexIndex();
		m_urlToIndex[url] = index;
		m_indexToUrl[index] = url;
		m_links.push_back(std::vector<Link>());
		return index;
	}

	std::string getUrl(Vertex vertex) const {
		if (vertex >= verticesNumber()) {
			throw std::invalid_argument("No such vertex in Webgraph, index: "
										+ std::to_string(vertex));
		}

		return m_indexToUrl.at(vertex);
	}

	Vertex urlToIndex(const std::string &url) const {
		if (!containsUrl(url)) {
			throw std::invalid_argument("No such vertex in Webgraph, url: " + url);
		}

		return m_urlToIndex.at(url);
	}

	void addLink(Vertex source, Vertex destination) {
        Log::debug("Adding link ", source, " ", destination);
		if (source >= verticesNumber()) {
			throw std::invalid_argument("No such vertex in Webgraph, source: "
										+ std::to_string(source));
		}

		if (destination >= verticesNumber()) {
			throw std::invalid_argument("No such vertex in Webgraph, destination: "
										+ std::to_string(destination));
		}

		m_links[source].push_back(Link(destination));
		++m_edgesNumber;
	}

	void addLink(const std::string &source, const std::string &destination) {
		Vertex sourceIndex = urlToIndex(source);
		Vertex destinationIndex = urlToIndex(destination);
		addLink(sourceIndex, destinationIndex);
	}

	Vertex newVertexIndex() const {
		return m_links.size();
	}

	const std::vector<Link> &getLinks(Vertex vertex) const {
		if (vertex >= verticesNumber()) {
			throw std::invalid_argument("No such vertex in Webgraph, index: "
										+ std::to_string(vertex));
		}

		return m_links[vertex];
	}

private:
	std::unordered_map<std::string, Vertex> m_urlToIndex;
	std::unordered_map<Vertex, std::string> m_indexToUrl;

	std::vector< std::vector<Link> > m_links;
	size_t m_edgesNumber;
};

} // namespace webgraph

#endif // WEBGRAPH_HPP