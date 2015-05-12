#ifndef FILE_SIMHASH_BUILDER_HPP
#define FILE_SIMHASH_BUILDER_HPP

#include <thread>
#include <stdexcept>
#include <sstream>

#include "filecrawler/concurrent_queue.hpp"

#include "html_utils.hpp"

namespace simhash {

using filecrawler::FileProcessor;
using filecrawler::ConcurrentQueue;

typedef uint64_t Simhash;

struct DocumentSimilarityInfo {
    DocumentSimilarityInfo(const std::string &path, const Simhash &simhash, size_t size): 
        path(path), simhash(simhash), size(size) {
    }

    std::string path;
    Simhash simhash;
    size_t size;
};

struct DocumentInfo {
    DocumentInfo(size_t id, DocumentSimilarityInfo similarity): id(id), 
        path(similarity.path), simhash(similarity.simhash), size(similarity.size) {}

    size_t id;
    // Flatten structure for simplification
    std::string path;
    Simhash simhash;
    size_t size;
};

class SimhashCalculator {
public:
    Simhash calculate(const std::string &text) {
        hashtable.assign(64, 0);

        std::vector<std::string> line;
        std::string word;
        for (size_t i = 0; i < text.size(); ++i) {
            if (isspace(text[i])) {
                line.push_back(word);
                word = "";
            } else {
                word += text[i];
            }
            if (text[i] == '\n') {
                calculatePhraseSimhash(line);
                line.clear();
            }
        }
        Simhash simhash = 0;
        for (size_t bit = 0; bit < 64; ++bit) {
            simhash <<= 1;
            simhash |= hashtable[bit] >= 0;
        }
        return simhash;
    }
private:
    void calculatePhraseSimhash(std::vector<std::string> line) {
        for (size_t i = 0; i + 1 < line.size(); ++i) {
            std::string hashed = line[i] + " " + line[i + 1];
            size_t hash = std::hash<std::string>()(hashed);
            for (size_t bit = 0; bit < 64; ++bit) {
                hashtable[bit] += (hash & (1ll << bit)) ? 1 : -1;
            }
        }
    }

    std::vector<int> hashtable;
};

class FileSimhashBuilder : public FileProcessor {
public:
    FileSimhashBuilder(ConcurrentQueue<std::string>& filesForProcessingQueue,
        std::vector<DocumentSimilarityInfo> &documentInfos, std::mutex &documentInfosMutex):
        FileProcessor(filesForProcessingQueue), documentInfos(documentInfos), 
        documentInfosMutex(documentInfosMutex) {
    }

    ~FileSimhashBuilder() {
    }

private:
    void mergeThreadResources() {
        std::lock_guard<std::mutex> guard(documentInfosMutex);
        for (const auto &documentInfo : threadDocumentsInfos) {
            documentInfos.push_back(documentInfo);
        }
        threadDocumentsInfos.clear();
    }

    bool process(const std::string& path) {
        Log::debug("Processing file ", path);

        std::ifstream infile;
        infile.open(path, std::ios::binary);

        if (!infile.is_open()) {
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

        std::string parsed_data;
        try {
            parsed_data = get_inner_text(data);
        } catch (std::exception &e) {
            Log::error("Failed to parse ", path, " Error: ", e.what());
        }

        // {
        //     std::ofstream ofs(path + ".parse");
        //     ofs << parsed_data << '\n';
        //     ofs.close();
        // }

        // Process data here
        SimhashCalculator simhashCalculator;

        threadDocumentsInfos.emplace_back(
            path, 
            simhashCalculator.calculate(parsed_data),
            parsed_data.size()
        );

        return true;
    }

    std::vector<DocumentSimilarityInfo> &documentInfos;
    std::vector<DocumentSimilarityInfo> threadDocumentsInfos;
    std::mutex &documentInfosMutex;
};

} // namespace simhash

#endif // FILE_SIMHASH_BUILDER_HPP
