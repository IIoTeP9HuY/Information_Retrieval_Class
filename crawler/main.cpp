#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>

#include <boost/program_options.hpp>

#include "crawler.hpp"

namespace po = boost::program_options;

std::shared_ptr<Crawler> crawler;

void interruptHandler(int param)
{
	std::cerr << "Interrupted" << std::endl;
	crawler->stop();
}

int main(int argc, const char* argv[]) {
	URL startURL;
	size_t maxDepth;
	size_t maxPages;
	std::string downloadDir;
	std::string urlsFilepath;
	size_t threadsNumber;
	bool debugOutput = false;

    po::options_description generic("Generic options");
    generic.add_options()
        ("help", "produce this help message")
        ("threads,t", po::value<size_t>(&threadsNumber)->default_value(3), "set number of threads")
        ("depth,d", po::value<size_t>(&maxDepth)->default_value(std::numeric_limits<size_t>::max()), "set max depth of crawling")
        ("pages,p", po::value<size_t>(&maxPages)->default_value(std::numeric_limits<size_t>::max()), "set max number of downloaded pages")
        ("dest,o", po::value<std::string>(&downloadDir)->default_value("./site"), "set download directory")
        ("verbose,v", "turn on verbose output")
        ("continue,c", "resume download")
    ;

    po::positional_options_description p;
    p.add("url", -1);

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("url", po::value<std::string>(&startURL), "start url")
    ;

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(hidden);

    po::variables_map vm;

    try
    {
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    }
    catch (po::error& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << "Usage: crawler PATH" << std::endl;
        std::cout << generic << std::endl;
        return 1;
    }

    if (threadsNumber == 0)
    {
        std::cerr << "Wrong number of threads" << std::endl;
    }

    if (!vm.count("url"))
    {
        std::cerr << "Usage: crawler URL" << std::endl;
        std::cerr << "Try 'crawler --help' for more information" << std::endl;
        return 1;
    }

    if (vm.count("verbose"))
    {
    	debugOutput = true;
    }

    void (*prev_handler)(int);

    crawler = std::make_shared<Crawler>(startURL, maxDepth, maxPages, downloadDir, threadsNumber, debugOutput);

    if (vm.count("continue"))
    {
    	{
    		std::ifstream ifs("new_urls.txt");
    		while (!ifs.eof())
    		{
    			std::string url;
    			ifs >> url;
    			crawler->addNewUrl(url);
    		}
    	}
    	{
    		std::ifstream ifs("ready_urls.txt");
    		while (!ifs.eof())
    		{
    			std::string url;
    			ifs >> url;
    			crawler->addOldUrl(url);
    		}
    	}
    }

    prev_handler = signal(SIGINT, interruptHandler);

	crawler->start();

	return 0;
}
