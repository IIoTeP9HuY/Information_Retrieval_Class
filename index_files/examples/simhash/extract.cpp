#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include <sstream>
#include <unordered_map>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "html_utils.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

typedef std::string URL;

void interruptHandler(int param)
{
    std::cerr << "Interrupted. Saving progress..." << std::endl;
    exit(0);
}

std::unordered_map<string, int> tokenFrequency;

void processFile(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream infile;
    infile.open(inputFile, std::ios::binary);

    if (!infile.is_open()) {
        std::cerr << "Failed to open file " << inputFile << std::endl;
        return;
    }

    infile.seekg(0, std::ios::end);
    size_t fileSizeInBytes = infile.tellg();

    std::string data;
    data.resize(fileSizeInBytes);
    infile.seekg(0, std::ios::beg);
    infile.read(&data[0], fileSizeInBytes);

    std::string parsed_data;
    try {
        parsed_data = get_inner_text(data);
    } catch (std::exception &e) {
        std::cerr << "Failed to parse " << inputFile << " Error: " << e.what() << std::endl;
    }

    std::ofstream ofs(outputFile);
    ofs << parsed_data << '\n';
    ofs.close();

    std::stringstream ss(parsed_data);
    string token;
    while (ss >> token) {
        ++tokenFrequency[token];
    }
}

int main(int argc, const char* argv[]) {
    std::string urlDir;
    std::string outputDir;
    std::string urlsMapping;
    bool debugOutput = false;

    po::options_description generic("Generic options");
    generic.add_options()
        ("help", "produce this help message")
        ("urlsDir", po::value<std::string>(&urlDir)->default_value("./flat_site"), "set web pages directory")
        ("outDir", po::value<std::string>(&outputDir)->default_value("./text_site"), "set path to save output files")
        ("urlsMapping", po::value<std::string>(&urlsMapping)->default_value("urls"), "set path to save urls mapping file")
        ("verbose,v", "turn on verbose output")
    ;

    po::variables_map vm;

    po::options_description cmdline_options;
    cmdline_options.add(generic);

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
        std::cout << "Usage: flatten PATH" << std::endl;
        std::cout << generic << std::endl;
        return 1;
    }

    if (vm.count("verbose"))
    {
        debugOutput = true;
    }

    void (*prev_handler)(int);

    std::ifstream urlsMappingStream(urlsMapping);
    if (!urlsMappingStream.is_open()) {
        std::cout << "Failed to read url mappings list" << std::endl;
        return 0;
    }

    try {
        boost::filesystem::create_directories(outputDir);
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }

    URL url;
    std::string filename;
    string newline;
    int urlsProcessed = 0;
    while (urlsMappingStream >> filename >> url) {
        fs::path outDir(outputDir);
        auto templateName = std::to_string(urlsProcessed + 1) + ".txt";
        fs::path templateFileName(templateName);
        fs::path newFilePath = outDir / templateFileName;

        try {
            fs::path fileName(filename);
            fs::path inputDirPath(urlDir);
            fs::path filePath = inputDirPath / fileName;
            processFile(filePath.string(), newFilePath.string());
            ++urlsProcessed;
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
        }

        if (urlsProcessed % 10000 == 0) {
            std::cerr << "Urls processed: " << urlsProcessed << std::endl;
        }
    }

    std::ofstream tokenFrequencyStream("token_frequency");
    for (const auto& frequency : tokenFrequency) {
        tokenFrequencyStream << frequency.first << "\t" << frequency.second << "\n";
    }

    prev_handler = signal(SIGINT, interruptHandler);

    return 0;
}
