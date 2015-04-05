#ifndef CRAWLER_CRAWLER_HPP
#define CRAWLER_CRAWLER_HPP

#include <atomic>
#include <thread>
#include <iterator>
#include <algorithm>

#include <curl/curl.h>

#include <boost/filesystem/operations.hpp>

#include "concurrent.hpp"
#include "url_utils.hpp"
#include "timer.hpp"

namespace NCrawler {

size_t string_write(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string *) userp)->append((char *) contents, size * nmemb);
	return size * nmemb;
}

CURLcode curl_read(const URL &url, std::string &buffer, long timeout = 15)
{
	Timer timer("Download: " + url);
	CURLcode code(CURLE_FAILED_INIT);
	CURL *curl = curl_easy_init();

	if (curl) {
		if (CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str()))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_write))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1))) {

			code = curl_easy_perform(curl);
		}
		curl_easy_cleanup(curl);
	}
	timer.stop();
	return code;
}

void writeToFile(std::string fileName, const std::string &content)
{
	std::ofstream file(fileName, std::ios::out | std::ios::trunc);
	file << content << std::endl;
}

void writePageToFile(URL url, const std::string &content, std::string downloadDir, bool verbose = false)
{
	url = preprocessURL(url);
	while ((!downloadDir.empty()) && (downloadDir.back() == '/'))
		downloadDir.resize(downloadDir.length() - 1);

	std::string filePath = downloadDir + "/" + addFileExtension(url);
	std::string dirPath = downloadDir + "/" + url;
	rTrimChar(dirPath, '/');
	try {
		boost::filesystem::create_directories(dirPath);
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	if (verbose) {
		std::cerr << "Filename: " << filePath << std::endl;
		std::cerr << "Writing to " << dirPath << std::endl;
	}

	writeToFile(filePath, content);
}

class Crawler
{
public:
	Crawler(URL startURL, size_t maxDepth, size_t maxPages,
			const std::string &downloadDir, size_t threadsNumber,
			bool debugOutput = false) :
			startURL(startURL), maxDepth(maxDepth), maxPages(maxPages),
			downloadDir(downloadDir), threadsNumber(threadsNumber),
			debugOutput(debugOutput)
	{
		finishedThreads.store(0);
		pagesDownloaded.store(0);
		pagesDownloadingNow.store(0);
		totalSize.store(0);
	}

	void addNewUrl(const std::string &url)
	{
		addUrlToQueue(url, 0);
	}

	void addOldUrl(const std::string &url)
	{
		addedToQueuePages.insert(url);
	}

	void start()
	{
		Timer timer("Total time");
		addUrlToQueue(startURL, 0);

		std::vector<std::thread> threads;

		if (debugOutput) {
			std::cerr << "threadsNumber: " << threadsNumber << std::endl;
		}

		for (std::size_t threadNumber = 0; threadNumber < threadsNumber;
			 ++threadNumber) {
			threads.push_back(std::thread(&Crawler::threadFunction, this));
		}
		for (std::size_t threadNumber = 0; threadNumber < threadsNumber;
			 ++threadNumber) {
			threads[threadNumber].join();
		}

		std::cout << "Total size: " << trunc(double(totalSize) / 1000) / 1000 << "mb" << std::endl;
		std::cout << "Pages downloaded: " << pagesDownloaded << std::endl;
		timer.stop();
	}

	void stop()
	{
		maxPages = 0;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::unordered_set<URL> notReadyUrls;
		{
			std::vector<URL> urls;
			while (!urlQueue.empty()) {
				auto url = urlQueue.pop().first;
				urls.push_back(url);
				notReadyUrls.insert(url);
			}

			std::ofstream os("new_urls.txt");
			for (int i = 0; i < urls.size(); ++i) {
				os << urls[i] << std::endl;
			}
			os.close();
		}
		{
			std::vector<URL> discoveredUrls = addedToQueuePages.values();
			std::vector<URL> urls;
			for (const auto &url : discoveredUrls) {
				if (notReadyUrls.find(url) == notReadyUrls.end()) {
					urls.push_back(url);
				}
			}
			std::ofstream os("ready_urls.txt");
			for (int i = 0; i < urls.size(); ++i) {
				os << urls[i] << std::endl;
			}
			os.close();
		}
	}

	void restore()
	{
    	{
    		std::ifstream ifs("new_urls.txt");
    		while (!ifs.eof())
    		{
    			std::string url;
    			ifs >> url;
    			addNewUrl(url);
    		}
    	}
    	{
    		std::ifstream ifs("ready_urls.txt");
    		while (!ifs.eof())
    		{
    			std::string url;
    			ifs >> url;
    			addOldUrl(url);
    		}
    	}
	}

private:

	void threadFunction()
	{
		bool isFinished = false;

		while ((pagesDownloaded.load() < maxPages) &&
			   ((finishedThreads.load() < threadsNumber) ||
				!urlQueue.empty())) {
			std::pair<URL, size_t> urlInfo;
			if (pagesDownloaded.load() + pagesDownloadingNow.load() < maxPages
				&& urlQueue.tryPop(urlInfo)) {
				if (isFinished) {
					isFinished = false;
					finishedThreads -= 1;
				}
				++pagesDownloadingNow;
				crawl(urlInfo.first, urlInfo.second);
				--pagesDownloadingNow;
			}
			else if (!isFinished) {
				isFinished = true;
				finishedThreads += 1;
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
		if (debugOutput) {
			std::cerr << "Thread: " << std::this_thread::get_id() << " finished" << std::endl;
		}
	}

	void crawl(URL url, size_t depth)
	{
		if (debugOutput) {
			std::cerr << "Url " << url << ", depth " << depth << std::endl;
		}
		std::string content;
		if (!isAllowed(startURL, url)) {
			return;
		}

		CURLcode res = curl_read(url, content);
		if (res == CURLE_OK) {
			++pagesDownloaded;
			writePageToFile(url, content, downloadDir, debugOutput);

			totalSize += content.size();

			if (depth + 1 <= maxDepth) {
				std::vector<URL> urls = getUrls(url, content);
				for (const auto &url : urls) {
					if (isAllowed(startURL, url)) {
						addUrlToQueue(url, depth + 1);
					}
				}
			}
		}
		else {
			if (debugOutput) {
				std::cerr << "ERROR: " << curl_easy_strerror(res) << std::endl;
			}
		}
	}

	bool addUrlToQueue(const URL &url, size_t depth)
	{
		bool inserted = addedToQueuePages.tryInsert(url);
		if (inserted) {
			urlQueue.push(std::make_pair(url, depth));
		}
		return inserted;
	}

	URL startURL;
	std::atomic<size_t> totalSize;
	std::atomic<size_t> pagesDownloaded;
	std::atomic<size_t> pagesDownloadingNow;
	std::atomic<size_t> finishedThreads;
	size_t threadsNumber;
	size_t maxDepth, maxPages;
	std::string downloadDir;
	ConcurrentQueue<std::pair<URL, size_t> > urlQueue;
	ConcurrentUnorderedSet<URL> addedToQueuePages;
	bool debugOutput;
};

} // namespace NCrawler

#endif // CRAWLER_CRAWLER_HPP
