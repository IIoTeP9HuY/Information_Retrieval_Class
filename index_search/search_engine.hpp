#ifndef SEARCH_ENGINE_HPP
#define SEARCH_ENGINE_HPP

#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cmath>

namespace irindexer {

using std::string;
using std::vector;
using std::cin;

class Dictionary {
public:
    void readFromFile(string filename)
    {
        std::ifstream input(filename);

        if (!input.is_open()) {
            throw std::logic_error("Can't open file " + filename);
        }

        std::cerr << "Reading dictionary from " << filename << std::endl;

        int wordsNumber = 0;
        while (!input.eof())
        {
            string word;
            int index, frequency;
            input >> word >> index >> frequency;
            addWord(word, index, frequency);
            ++wordsNumber;
        }

        std::cerr << "Finished reading " << std::to_string(wordsNumber) << " words" << std::endl;
    }

    struct WordRecord {
        WordRecord() {}

        WordRecord(string word, int index, int frequency): 
            word(word), index(index), frequency(frequency)
        {
        }

        string word;
        int index;
        int frequency;
    };

    size_t size() const {
        return intToRecord.size();
    }

    void addWord(string word, int index, int frequency)
    {
        WordRecord record(word, index, frequency);
        intToRecord[index] = record;
        wordToRecord[word] = record;
    }

    bool containsWord(string word) {
        return (wordToRecord.find(word) != wordToRecord.end());
    }

    WordRecord getWordRecord(int index) const {
        return intToRecord.at(index);
    }

    WordRecord getWordRecord(string word) const {
        return wordToRecord.at(word);
    }

private:
    std::unordered_map<int, WordRecord> intToRecord;
    std::unordered_map<string, WordRecord> wordToRecord;
};

class Index {
public:
    Index(): averageDocumentLength(0.0) {}

    void readFromFile(string filename) {
        std::ifstream input(filename);

        if (!input.is_open()) {
            throw std::logic_error("Can't open file " + filename);
        }

        std::cerr << "Reading index from " << filename << std::endl;

        while (!input.eof()) {
            int wordIndex;
            input >> wordIndex;
            string line;
            std::getline(input, line);
            std::stringstream stream(line);
            int documentIndex;
            char separator;
            int frequency;
            while (stream >> documentIndex >> separator >> frequency) {
                addWordDocument(wordIndex, documentIndex, frequency);
                documents.insert(documentIndex);
                updateMaxWordDocumentFrequency(documentIndex, frequency);
                averageDocumentLength += frequency;
            }
            averageDocumentLength /= documentsNumber();
        }

        std::cerr << "Finished reading, " << documents.size() << " documents" << std::endl;
    }

    size_t documentsNumber() const {
        return documents.size();
    }

    size_t getWordDocumentsNumber(int wordIndex) const {
        return wordDocumentFrequency.at(wordIndex).size();
    }

    vector<int> getWordDocuments(int wordIndex) const {
        vector<int> documents;
        for (const auto &doc : wordDocumentFrequency.at(wordIndex)) {
            documents.push_back(doc.first);
        }
        return documents;
    }

    void addWordDocument(int wordIndex, int documentIndex, int frequency) {
        wordDocumentFrequency[wordIndex][documentIndex] = frequency;
    }

    int getWordDocumentFrequency(int wordIndex, int documentIndex) const {
        return wordDocumentFrequency.at(wordIndex).at(documentIndex);
    }

    int getMaxWordDocumentFrequency(int documentIndex) const {
        return maxWordDocumentFrequency.at(documentIndex);
    }

    double getAverageDocumentLength() const {
        return averageDocumentLength;
    }

private:

    void updateMaxWordDocumentFrequency(int documentIndex, int frequency) {
        maxWordDocumentFrequency[documentIndex] = std::max(maxWordDocumentFrequency[documentIndex], frequency);
    }

    double averageDocumentLength;
    std::unordered_set<int> documents;
    std::unordered_map< int, std::unordered_map<int, int> > wordDocumentFrequency;
    std::unordered_map<int, int> maxWordDocumentFrequency;
};

vector<string> tokenize(const string &text, const string &delimiters) {
    std::unordered_set<char> delimiters_set(delimiters.begin(), delimiters.end());
    vector<string> tokens;
    string currentToken = "";
    for (size_t i = 0; i <= text.length(); ++i) {
        char c = text[i];
        if ((i != text.length()) && (delimiters_set.find(c) == delimiters_set.end())) {
            currentToken += c;
        } else {
            if (!currentToken.empty()) {
                tokens.push_back(currentToken);
            }
            currentToken = "";
        }
    }
    return tokens;
}

const string delimeters = " ,\n\t";

class SearchEngine {
public:
    SearchEngine() {}

    SearchEngine(const Dictionary &dict, const Index &index): dict(dict), index(index) {}

    SearchEngine(const string& dictPath, const string& indexPath) {
        dict.readFromFile(dictPath);
        index.readFromFile(indexPath);
    }

    struct DocumentScore {
        DocumentScore(double score, int documentIndex): score(score), documentIndex(documentIndex) {}

        DocumentScore(): DocumentScore(0, 0) {}

        bool operator < (const DocumentScore &ds) const {
            return score > ds.score;
        }

        double score;
        int documentIndex;
    };

    template<typename DocumentScoreEvaluator>
    vector<DocumentScore> ScoredPhraseSearch(string phrase) {
        DocumentScoreEvaluator evaluator(dict, index);

        std::cerr << "Using " << evaluator.getName() << std::endl;

        vector<Dictionary::WordRecord> tokensRecords = transformPhrase(phrase);
        vector<int> documents = findDocumentsIntersection(tokensRecords);

        std::cerr << "Found " << documents.size() << " documents" << std::endl;

        vector<DocumentScore> documentScores;
        for (int document : documents) {
            double score = evaluator.evaluateScore(document, tokensRecords);
            documentScores.push_back(DocumentScore(score, document));
        }
        std::sort(documentScores.begin(), documentScores.end());
        return documentScores;
    }

private:

    vector<Dictionary::WordRecord> transformPhrase(string phrase) {
        vector<Dictionary::WordRecord> tokensRecords;
        vector<string> tokens = tokenize(phrase, delimeters);

        if (tokens.empty()) {
            return tokensRecords;
        }

        for (size_t i = 0; i < tokens.size(); ++i) {
            if (!dict.containsWord(tokens[i])) {
                return vector<Dictionary::WordRecord>();
            }
            tokensRecords.push_back(dict.getWordRecord(tokens[i]));
        }
        return tokensRecords;
    }

    vector<int> findDocumentsIntersection(const vector<Dictionary::WordRecord>& records) {
        vector<int> searchResults;

        if (records.empty()) {
            return searchResults;
        }
        searchResults = index.getWordDocuments(records[0].index);
        for (size_t i = 1; i < records.size(); ++i) {
            vector<int> wordDocuments = index.getWordDocuments(records[i].index);
            std::unordered_set<int> wordDocumentsSet(wordDocuments.begin(), wordDocuments.end());

            vector<int> filteredSearchResults;
            for (const auto &result : searchResults) {
                if (wordDocumentsSet.find(result) != wordDocumentsSet.end()) {
                    filteredSearchResults.push_back(result);
                }
            }
            searchResults = filteredSearchResults;
        }
        return searchResults;
    }

    Dictionary dict;
    Index index;
};

class DocumentScoreEvaluator {
public:
    DocumentScoreEvaluator(const Dictionary &dict, const Index &index): dict(dict), index(index) {}

    virtual double evaluateScore(int documentIndex, const vector<Dictionary::WordRecord>& keywords) const {
        return 1.0;
    }

    virtual string getName() const {
        return "ScoreEvaluator";
    }

protected:
    Dictionary dict;
    Index index;
};

class BooleanDocumentScoreEvaluator : public DocumentScoreEvaluator {
public:
    BooleanDocumentScoreEvaluator(const Dictionary &dict, const Index &index): DocumentScoreEvaluator(dict, index) {}

    double evaluateScore(int documentIndex, const vector<Dictionary::WordRecord>& keywords) const {
        return 1;
    }

    string getName() const {
        return "Boolean ScoreEvaluator";
    }
};

class TFIDFDocumentScoreEvaluator : public DocumentScoreEvaluator {
public:
    TFIDFDocumentScoreEvaluator(const Dictionary &dict, const Index &index): DocumentScoreEvaluator(dict, index) {}

    double evaluateScore(int documentIndex, const vector<Dictionary::WordRecord>& keywords) const {
        double score = 0;
        for (size_t i = 0; i < keywords.size(); ++i) {
            size_t wordDocumentsNumber = index.getWordDocumentsNumber(keywords[i].index);
            size_t wordDocumentFrequency = index.getWordDocumentFrequency(keywords[i].index, documentIndex);
            double idf = log((index.documentsNumber() - wordDocumentsNumber + 0.5) 
                    / (wordDocumentsNumber + 0.5));

            double tf = 0.5 + 0.5 * wordDocumentFrequency / index.getMaxWordDocumentFrequency(documentIndex);

            score += idf * tf;
        }
        return score;
    }

    string getName() const {
        return "TFIDF ScoreEvaluator";
    }
};

class BM25DocumentScoreEvaluator : public DocumentScoreEvaluator {
public:
    BM25DocumentScoreEvaluator(const Dictionary &dict, const Index &index): DocumentScoreEvaluator(dict, index) {}

    double evaluateScore(int documentIndex, const vector<Dictionary::WordRecord>& keywords) const {
        double b = 0.75;
        double k = 1.5;
        double score = 0;
        for (size_t i = 0; i < keywords.size(); ++i) {
            size_t wordDocumentsNumber = index.getWordDocumentsNumber(keywords[i].index);

            double wordDocumentFrequency = index.getWordDocumentFrequency(keywords[i].index, documentIndex) * 1.0 
                / index.getMaxWordDocumentFrequency(documentIndex);

            double idf = log((index.documentsNumber() - wordDocumentsNumber + 0.5) 
                    / (wordDocumentsNumber + 0.5));

            score += idf * (wordDocumentFrequency * (k + 1)) 
                / (wordDocumentFrequency + k * (1 - b + b * index.documentsNumber() / index.getAverageDocumentLength()));
        }
        return score;
    }

    string getName() const {
        return "BM25 ScoreEvaluator";
    }
};

} // namespace irindexer

#endif // SEARCH_ENGINE_HPP
