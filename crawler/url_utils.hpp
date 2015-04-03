#ifndef CRAWLER_URL_UTILS_HPP
#define CRAWLER_URL_UTILS_HPP

#include <boost/regex.hpp>

namespace NCrawler {

typedef std::string URL;

boost::regex link_regex("<\\s*A\\s+[^>]*href\\s*=\\s*\"(http://|https://)?([^\"]*)\"",
						boost::regex::normal | boost::regbase::icase);

boost::regex url_regex("(http://|https://)?([^\"]*)",
					   boost::regex::normal | boost::regbase::icase);

boost::regex domain_regex("(http://|https://)?([^\"/]*)",
						  boost::regex::normal | boost::regbase::icase);

boost::regex html_extension_regex(".html",
								  boost::regex::normal | boost::regbase::icase);

boost::regex bad_extension_regex(".(xml|php|js|jpg|png)",
								 boost::regex::normal | boost::regbase::icase);

boost::regex subsection_regex(
		"(Special|User_talk|User|Wikipedia_talk|User|Template|MediaWiki|Talk|Wikipedia|Help|File):",
		boost::regex::normal | boost::regbase::icase);

static inline std::string &rTrimChar(std::string &s, char c)
{
	s.erase(std::find(s.rbegin(), s.rend(), c).base(), s.end());
	return s;
}

URL domain(const URL &url)
{
	boost::smatch matches;
	boost::regex_search(url, matches, domain_regex);
	return std::string(matches[0].first, matches[0].second);
}

bool goodFileExtension(URL url)
{
	boost::smatch matches;
	return !boost::regex_search(url, matches, bad_extension_regex);
}

bool noSubsection(URL url)
{
	boost::smatch matches;
	return !boost::regex_search(url, matches, subsection_regex);
}

bool noHashtag(URL url)
{
	return url.find("#") == std::string::npos;
}

bool noColon(URL url)
{
	return url.find(":") == std::string::npos;
}

bool noQuestionMark(URL url)
{
	return url.find("?") == std::string::npos;
}

URL addFileExtension(URL url)
{
	boost::smatch matches;
	if (!boost::regex_search(url, matches, html_extension_regex)) {
		url += ".html";
	}
	return url;
}

bool startsWith(const std::string &s, const std::string start)
{
	if (start.length() > s.length()) {
		return false;
	}
	return !s.compare(0, start.length(), start);
}

bool isAllowed(URL startURL, URL url)
{
	return ((domain(url) == domain(startURL))
			&& goodFileExtension(url)
			&& noHashtag(url)
			&& noSubsection(url)
			&& noQuestionMark(url));
}

std::vector<URL> getUrls(URL rootURL, const std::string &content)
{
	std::vector<URL> urls;

	boost::sregex_iterator it(content.begin(), content.end(), link_regex);
	boost::sregex_iterator end;

	while (rootURL.back() == '/') {
		rootURL.resize(rootURL.length() - 1);
	}

	while (it != end) {
		auto matches = *it;
		std::string http(matches[1].first, matches[1].second);
		URL url(matches[2].first, matches[2].second);

		while (!url.empty() && url.back() == '/') {
			url.resize(url.length() - 1);
		}

		if (http.empty()) {
			if (startsWith(url, "mailto")) {
				url = "";
			}
			else if (startsWith(url, "//")) {
				url = "http://" + url.substr(2);
			}
			else if (startsWith(url, "/")) {
				if (domain(rootURL).length() == 0) {
					continue;
				}
				url = domain(rootURL) + url;
			}
			else {
				URL previousPageURL = rootURL;
				rTrimChar(previousPageURL, '/');
				while (!previousPageURL.empty() && previousPageURL.back() == '/') {
					previousPageURL.resize(previousPageURL.length() - 1);
				}
				if (!previousPageURL.empty()) {
					url = previousPageURL + "/" + url;
				}
			}
		}
		else {
			url = http + url;
		}
		if (!url.empty()) {
			urls.push_back(url);
		}
		++it;
	}

	return urls;
}

std::string preprocessURL(URL url, bool verbose = false)
{
	boost::smatch matches;
	if (boost::regex_search(url, matches, url_regex)) {
		url = std::string(matches[2].first, matches[2].second);
	}

	while (url.front() == '/')
		url.erase(url.begin());

	while (url.back() == '/')
		url.resize(url.length() - 1);

	return url;
}

} // namespace NCrawler

#endif // CRAWLER_URL_UTILS_HPP
