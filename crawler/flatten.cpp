#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>

#include <boost/program_options.hpp>

#include "crawler.hpp"

using namespace NCrawler;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

void interruptHandler(int param)
{
    std::cerr << "Interrupted. Saving progress..." << std::endl;
    exit(0);
}

int main(int argc, const char* argv[]) {
    std::string urlDir;
    std::string urlsList;
    std::string outputDir;
    std::string urlsMapping;
    bool debugOutput = false;

    po::options_description generic("Generic options");
    generic.add_options()
        ("help", "produce this help message")
        ("urlsDir", po::value<std::string>(&urlDir)->default_value("./site"), "set web pages directory")
        ("urlsList", po::value<std::string>(&urlsList)->default_value("ready_urls.txt"), "set list of downloaded urls")
        ("outDir", po::value<std::string>(&outputDir)->default_value("./flat_site"), "set path to save output files")
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

    std::ifstream urlsListStream(urlsList);
    if (!urlsListStream.is_open()) {
        std::cout << "Failed to read urls list" << std::endl;
        return 0;
    }

    std::ofstream urlsMappingStream(urlsMapping);
    if (!urlsMappingStream.is_open()) {
        std::cout << "Failed to write to urls list" << std::endl;
        return 0;
    }

    try {
        boost::filesystem::create_directories(outputDir);
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }

    URL url;
    int urlsProcessed = 0;
    while (urlsListStream >> url) {
        std::string rawFilePath;
        std::string rawDirPath;
        std::tie(rawFilePath, rawDirPath) = urlToPath(url, urlDir);

        fs::path filePath(rawFilePath);
        fs::path outDir(outputDir);
        auto templateName = std::to_string(urlsProcessed + 1) + ".html";
        fs::path fileName(templateName);
        fs::path newFilePath = outDir / fileName;

        try {
            fs::copy(filePath, newFilePath);
            urlsMappingStream << templateName << '\t' << url << std::endl;
            ++urlsProcessed;
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
        }

        if (urlsProcessed % 10000 == 0) {
            std::cerr << "Urls processed: " << urlsProcessed << std::endl;
        }
    }

    prev_handler = signal(SIGINT, interruptHandler);

    return 0;
}
