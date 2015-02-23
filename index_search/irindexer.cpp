#include <iostream>
#include <cstdlib>
#include <cstdio>

#include "search_engine.hpp"

using namespace irindexer;

void printTop(const std::vector<SearchEngine::DocumentScore> &phraseDocuments, size_t topNumber) {
    for (size_t i = 0; i < std::min(topNumber, phraseDocuments.size()); ++i) {
        std::cout << "id: " << std::setw(5) << phraseDocuments[i].documentIndex
                  << "  score: " << phraseDocuments[i].score << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " DICTIONARY_FILE INDEX_FILE " << std::endl;
        return 0;
    }

    std::string dictPath(argv[1]);
    std::string indexPath(argv[2]);

    SearchEngine searchEngine(dictPath, indexPath);

    while (!feof(stdin)) {
        std::cout << "Search query: ";
        std::string searchPhrase;
        std::getline(std::cin, searchPhrase);
        std::cout << std::endl;

        if (feof(stdin)) {
            break;
        }

        printTop(searchEngine.ScoredPhraseSearch<TFIDFDocumentScoreEvaluator>(searchPhrase), 10);
        printTop(searchEngine.ScoredPhraseSearch<BM25DocumentScoreEvaluator>(searchPhrase), 10);

        std::cout << "--------------------------------" << std::endl;
    }

    return 0;
}
