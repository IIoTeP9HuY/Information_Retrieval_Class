#ifndef CLUSTERS_BUILDER_HPP
#define CLUSTERS_BUILDER_HPP

#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <fstream>

#include "filecrawler/logger.hpp"

namespace std {
template <> struct hash<std::pair<size_t, size_t>> {
    inline size_t operator()(const std::pair<size_t, size_t> &v) const {
        std::hash<size_t> hasher;
        return hasher(v.first) ^ hasher(v.second);
    }
};
}

namespace simhash {

size_t simhashDistance(const Simhash &lhs, const Simhash &rhs) {
    return __builtin_popcount(lhs ^ rhs);
}

uint64_t bitRotateRight(uint64_t x, size_t shift) {
    return (x << shift) | (x >> (64 - shift));
}

bool bitRotateComparator(const DocumentInfo &lhs, const DocumentInfo &rhs, const size_t &shift) {
    return (bitRotateRight(lhs.simhash, shift) < bitRotateRight(rhs.simhash, shift));
}

uint64_t bitDrop(uint64_t x, size_t bit) {
	if (bit >= 64) {
		return x;
	}
	return x & (~(1ll << bit));
}

bool bitDropComparator(const DocumentInfo &lhs, const DocumentInfo &rhs, const size_t &bit) {
    return (bitDrop(lhs.simhash, bit) < bitDrop(rhs.simhash, bit));
}

using namespace std::placeholders;

class ClustersBuilder {
public:
	ClustersBuilder(size_t simhashBitsDistance): simhashBitsDistance(simhashBitsDistance) {}

	std::vector<std::vector<size_t>> build(std::vector<DocumentInfo> documentInfos) {
		clusters.clear();

		logging::Log::info("Clustering ", documentInfos.size(), " documents");

		for (int bit = 64; bit >= 0; --bit) {
	        std::sort(documentInfos.begin(), documentInfos.end(), std::bind(bitDropComparator, _1, _2, bit));
	        std::vector<DocumentInfo> uniqueDocumentInfos;
	        size_t blockStart = 0;
	        Simhash blockSimhash = documentInfos[blockStart].simhash;
	        std::vector<size_t> currentBlock;
	        for (size_t i = 1; i <= documentInfos.size(); ++i) {
	        	if ((i == documentInfos.size()) || (bitDrop(blockSimhash, bit) != bitDrop(documentInfos[i].simhash, bit))) {
	        		uniqueDocumentInfos.push_back(documentInfos[blockStart]);
	        		if (!currentBlock.empty()) {
	        			sameSimhashes.push_back(
	        				std::make_pair(documentInfos[blockStart].id, currentBlock)
	        			);
	        		}
	        		currentBlock.clear();

	        		if (i != documentInfos.size()) {
	        			blockStart = i;
	        			blockSimhash = documentInfos[blockStart].simhash;
	        		}
	        	} else {
	        		currentBlock.push_back(documentInfos[i].id);
	        	}
	        }
			logging::Log::info("Unique: ", uniqueDocumentInfos.size(), ", current bit: ", bit);
	        documentInfos = std::move(uniqueDocumentInfos);
	    }

	    for (size_t i = 0; i < documentInfos.size(); ++i) {
	    	clusterOfDocument[documentInfos[i].id] = i;
	    	clusters.push_back({documentInfos[i].id});
	    }

        fillClusters();
		return clusters;
	}

	void fillClusters() {
		for (const auto &sameSimhashesBlock : sameSimhashes) {
			size_t id = sameSimhashesBlock.first;
			size_t clusterIndex = clusterOfDocument[id];
			for (auto duplicateId : sameSimhashesBlock.second) {
				clusters[clusterIndex].push_back(duplicateId);
			}
		}
	}
private:
	size_t simhashBitsDistance;
	std::vector<std::pair<size_t, std::vector<size_t>>> sameSimhashes;
	std::unordered_map<size_t, size_t> clusterOfDocument;
	std::vector<std::vector<size_t>> clusters;

	std::vector<std::unordered_set<size_t>> findSimilar(std::vector<DocumentInfo> &documentInfos, size_t simhashBitsDistance) {
	    std::vector<std::unordered_set<size_t>> similarDocuments(documentInfos.size());
	    std::vector<size_t> distancesHistogram(64, 0);
	    std::unordered_set<std::pair<size_t, size_t>> compared;

	    const size_t WINDOW_SIZE = 20;
	    const size_t ROTATE_SIZE = 8;

	    for (size_t k = 0; k < 64 / ROTATE_SIZE; ++k) {
	        std::sort(documentInfos.begin(), documentInfos.end(), 
	                std::bind(bitRotateComparator, _1, _2, k * ROTATE_SIZE));

	        Log::info("Rotate number: ", k);

	        for (size_t i = 0; i < documentInfos.size(); ++i) {
	            for (size_t j = i + 1; j < std::min(i + WINDOW_SIZE, documentInfos.size()); ++j) {
	                size_t s1 = documentInfos[i].size;
	                size_t s2 = documentInfos[j].size;
	                size_t idI = documentInfos[i].id;
	                size_t idJ = documentInfos[j].id;
	                double proportion = 0.25;
	                if (std::max(s1, s2) > std::min(s1, s2) * (1 + proportion)) {
	                    continue;
	                }
	                size_t distance = simhashDistance(documentInfos[i].simhash, documentInfos[j].simhash);
	                if (compared.find(std::make_pair(idI, idJ)) == compared.end()) {
	                    compared.insert(std::make_pair(idI, idJ));
	                    compared.insert(std::make_pair(idJ, idI));
	                    ++distancesHistogram[distance];
	                }
	                if (distance <= simhashBitsDistance) {
	                    if (similarDocuments[idI].find(idJ) == similarDocuments[idI].end()) {
	                        similarDocuments[idI].insert(idJ);
	                        similarDocuments[idJ].insert(idI);
	                    }
	                }
	            }
	        }
	    }

	    {
	        std::ofstream ofs("distances_histogram");
	        for (size_t i = 0; i < distancesHistogram.size(); ++i) {
	            ofs << i << " " << distancesHistogram[i] << '\n';
	        }
	        ofs.close();
	    }

	    return similarDocuments;
	}

	std::vector<std::vector<size_t>> findClusters(const std::vector<std::unordered_set<size_t>> &similarDocuments) {
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
	        for (size_t similarDocument : similarDocuments[document]) {
	            if (clustered[similarDocument]) {
	                continue;
	            }
	            clustered[similarDocument] = true;

	            for (size_t similarToSimilarDocument : similarDocuments[similarDocument]) {
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
};

} // namespace simhash

#endif // CLUSTERS_BUILDER_HPP