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
    return __builtin_popcountll(lhs ^ rhs);
}

uint64_t bitRotateRight(uint64_t x, size_t shift) {
    return (x << shift) | (x >> (64 - shift));
}

bool bitRotateComparator(const DocumentInfo &lhs, const DocumentInfo &rhs, const size_t &shift) {
    return (bitRotateRight(lhs.simhash, shift) < bitRotateRight(rhs.simhash, shift));
}

uint64_t bitDrop(uint64_t x, size_t bit) { if (bit >= 64) {
        return x;
    }
    return x & (~(1ll << bit));
}

bool bitDropComparator(const DocumentInfo &lhs, const DocumentInfo &rhs, const size_t &bit) {
    return (bitDrop(lhs.simhash, bit) < bitDrop(rhs.simhash, bit));
}

bool preprocessedSimhashComparator(const DocumentInfo &lhs, const DocumentInfo &rhs,
                                    std::function<uint64_t(uint64_t)> preprocessor) {
    return preprocessor(lhs.simhash) < preprocessor(rhs.simhash);
}

using namespace std::placeholders;

class ClustersBuilder {
public:
    ClustersBuilder(size_t simhashBitsDistance): simhashBitsDistance(simhashBitsDistance) {}

    std::vector<std::vector<size_t>> build(std::vector<DocumentInfo> documentInfos) {
        clusters.clear();

        logging::Log::info("Clustering ", documentInfos.size(), " documents");

        for (int firstBit = 64; firstBit >= 64; --firstBit) {
            for (int secondBit = firstBit; secondBit >= firstBit; --secondBit) {
                auto preprocessor = [&](uint64_t x) {
                    return bitDrop(bitDrop(x, firstBit), secondBit);
                };
                std::sort(documentInfos.begin(), documentInfos.end(),
                        std::bind(preprocessedSimhashComparator, _1, _2, preprocessor));
                std::vector<DocumentInfo> uniqueDocumentInfos;
                size_t blockStart = 0;
                Simhash blockSimhash = documentInfos[blockStart].simhash;
                std::string blockPath = documentInfos[blockStart].path;
                std::vector<size_t> currentBlock;
                for (size_t i = 1; i <= documentInfos.size(); ++i) {
                    if ((i == documentInfos.size()) || (preprocessor(blockSimhash) != preprocessor(documentInfos[i].simhash))) {
                        uniqueDocumentInfos.push_back(documentInfos[blockStart]);
                        if (!currentBlock.empty()) {
                            sameSimhashes.push_back(
                                std::make_pair(documentInfos[blockStart].id, currentBlock));
                        }
                        currentBlock.clear();

                        if (i != documentInfos.size()) {
                            blockStart = i;
                            blockSimhash = documentInfos[blockStart].simhash;
                            blockPath = documentInfos[blockStart].path;
                        }
                    } else {
                        currentBlock.push_back(documentInfos[i].id);
                    }
                }
                documentInfos = std::move(uniqueDocumentInfos);
            }
            logging::Log::info("Unique: ", documentInfos.size(), ", current bit ", firstBit);
        }

        auto similarDocument = findSimilar(documentInfos);
        clusters = findClusters(similarDocument);
        decodeIds(documentInfos);

        Log::info("Clusters number: ", clusters.size());
        for (size_t i = 0; i < clusters.size(); ++i) {
            for (size_t j = 0; j < clusters[i].size(); ++j) {
                clusterOfDocument[clusters[i][j]] = i;
            }
        }
        fillClusters();

        return clusters;
    }

    void fillClusters() {
        Log::info("Merging documents with same simhashes");
        for (int i = static_cast<int>(sameSimhashes.size()) - 1; i >= 0; --i) {
            const auto &sameSimhashesBlock = sameSimhashes[i];
            size_t id = sameSimhashesBlock.first;
            assert(clusterOfDocument.find(id) != clusterOfDocument.end());
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

    std::vector<std::unordered_set<size_t>> findSimilar(std::vector<DocumentInfo> &documentInfos) {
        std::vector<std::unordered_set<size_t>> similarDocuments(documentInfos.size());
        std::vector<size_t> distancesHistogram(64, 0);
        std::unordered_set<std::pair<size_t, size_t>> compared;

        const size_t WINDOW_SIZE = 30;
        const size_t ROTATE_SIZE = 4;

        for (size_t k = 0; k < 64 / ROTATE_SIZE; ++k) {
            std::sort(documentInfos.begin(), documentInfos.end(),
                    std::bind(bitRotateComparator, _1, _2, k * ROTATE_SIZE));

            Log::info("Rotate number: ", k);
            int similarFound = 0;

            for (size_t i = 0; i < documentInfos.size(); ++i) {
                for (size_t j = i + 1; j < std::min(i + WINDOW_SIZE, documentInfos.size()); ++j) {
                    size_t s1 = documentInfos[i].size;
                    size_t s2 = documentInfos[j].size;
                    // size_t idI = documentInfos[i].id;
                    // size_t idJ = documentInfos[j].id;
                    size_t idI = i;
                    size_t idJ = j;
                    double proportion = 0.20;
                    if (std::max(s1, s2) > std::min(s1, s2) * (1 + proportion)) {
                        continue;
                    }
                    if (compared.find(std::make_pair(idI, idJ)) == compared.end()) {
                        compared.insert(std::make_pair(idI, idJ));
                        compared.insert(std::make_pair(idJ, idI));
                        size_t distance = simhashDistance(documentInfos[i].simhash, documentInfos[j].simhash);

                        ++distancesHistogram[distance];
                        if (distance <= simhashBitsDistance) {
                            assert(similarDocuments[idI].find(idJ) == similarDocuments[idI].end());
                            similarDocuments[idI].insert(idJ);
                            similarDocuments[idJ].insert(idI);
                            ++similarFound;
                        }
                    }
                }
            }

            Log::info("Similar found: ", similarFound);
        }

        {
            std::ofstream ofs("stats/distances_histogram_" + std::to_string(simhashBitsDistance));
            for (size_t i = 0; i < distancesHistogram.size(); ++i) {
                ofs << i << " " << distancesHistogram[i] << '\n';
            }
            ofs.close();
        }

        return similarDocuments;
    }

    std::vector<std::vector<size_t>> findClusters(const std::vector<std::unordered_set<size_t>> &similarDocuments) {
        Log::info("Looking for clusters among ", similarDocuments.size(), " documents");
        std::vector<std::vector<size_t>> clusters;
        std::set<std::pair<size_t, size_t>, std::greater<std::pair<int, int>>> verticesPowers;
        std::vector<char> clustered(similarDocuments.size(), false);
        std::vector<size_t> currentVertexPower(similarDocuments.size(), 0);

        size_t maxVertexPower = 0;
        for (size_t i = 0; i < similarDocuments.size(); ++i) {
            verticesPowers.insert(std::make_pair(similarDocuments[i].size(), i));
            currentVertexPower[i] = similarDocuments[i].size();
            maxVertexPower = std::max(maxVertexPower, currentVertexPower[i]);
        }
        Log::info("Max vertex power: ", maxVertexPower);

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

    void decodeIds(std::vector<DocumentInfo> &documentInfos) {
        Log::info("Decoding ids");
        for (size_t i = 0; i < clusters.size(); ++i) {
            for (size_t j = 0; j < clusters[i].size(); ++j) {
                clusters[i][j] = documentInfos[clusters[i][j]].id;
            }
        }
    }
};

} // namespace simhash

#endif // CLUSTERS_BUILDER_HPP
