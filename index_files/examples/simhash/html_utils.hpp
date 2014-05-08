#ifndef HTML_UTILS_HPP
#define HTML_UTILS_HPP

#include <tidy/tidy.h>
#include <tidy/buffio.h>

#include <libxml++/libxml++.h>

#include "filecrawler/logger.hpp"

std::string html_xml(std::string html) {
	TidyDoc tidyDoc = tidyCreate();
	TidyBuffer tidyOutputBuffer = {0};
 
	bool configSuccess = tidyOptSetBool(tidyDoc, TidyXmlOut, yes) 
		&& tidyOptSetBool(tidyDoc, TidyQuiet, yes) 
		&& tidyOptSetBool(tidyDoc, TidyQuoteNbsp, no)
		&& tidyOptSetBool(tidyDoc, TidyXmlDecl, yes) //XML declaration on top of the content
		&& tidyOptSetBool(tidyDoc, TidyForceOutput, yes)
		&& tidyOptSetValue(tidyDoc, TidyInCharEncoding, "utf8") // Output from here should be UTF-8
		&& tidyOptSetValue(tidyDoc, TidyOutCharEncoding, "utf8") // Output from CURL is UTF-8
		&& tidyOptSetBool(tidyDoc, TidyNumEntities, yes) 
		&& tidyOptSetBool(tidyDoc, TidyShowWarnings, no) 
		&& tidyOptSetInt(tidyDoc, TidyDoctypeMode, TidyDoctypeOmit); //Exclude DOCTYPE
 
	int tidyResponseCode = -1;
 
	if (configSuccess) {
		std::vector<unsigned char> bytes(html.begin(), html.end());
 
		TidyBuffer buf;
		tidyBufInit(&buf);
 
		for(size_t i = 0; i < bytes.size(); i++) {
			tidyBufAppend(&buf, &bytes[i], 1);
		}
 
		tidyResponseCode = tidyParseBuffer(tidyDoc, &buf);
	}
 
	if (tidyResponseCode >= 0)
		tidyResponseCode = tidyCleanAndRepair(tidyDoc);
 
	if (tidyResponseCode >= 0)
		tidyResponseCode = tidySaveBuffer(tidyDoc, &tidyOutputBuffer);
 
	if (tidyResponseCode < 0) {
		throw ("Tidy encountered an error while parsing an HTML response. Tidy response code: " + tidyResponseCode);
	}
 
	std::string tidyResult = (char*) tidyOutputBuffer.bp;
 
	tidyBufFree(&tidyOutputBuffer);
	tidyRelease(tidyDoc);
 
	return tidyResult;
}

bool is_dead_character(int c) {
	return (c == '\n' || c == '\r' || c == '\t' || c == 0x20);
}

bool is_dead_string(std::string is) {
	bool r = true;

	std::string::iterator it(is.begin());
	std::string::iterator end(is.end());

	for ( ; it != end; ++it) {
		if(!is_dead_character(*it)) {
			r = false;
			break;
		}
	}

	return r;
}

bool is_punctuation(char c) {
	return (c == ',' || c == ',' || c == ':' || c == '?' || c == '!');
}

bool should_strip(char c) {
	return isspace(c) || ispunct(c);
}

void rstrip(std::string &s) {
	while (!s.empty() && should_strip(s.back())) {
		s.pop_back();
	}
}

void lstrip(std::string &s) {
	size_t pos = 0;
	while ((pos < s.length()) && should_strip(s[pos])) {
		++pos;
	}
	s = s.substr(pos);
}

void strip(std::string &s) {
	rstrip(s);
	lstrip(s);
}

std::string get_inner_text(xmlpp::Node* node, int depth = 0) {
	std::string ret;

	xmlpp::Node::NodeList list = node->get_children();

	for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter) {
		// I needed line breaks, if you don't, change this
		// if((*iter)->get_name().compare("br") == 0) {
		// 	ret += "<br>";
		// 	continue;
		// }

		// I would remove this line if you really want to capture everything
		// if((*iter)->get_name().compare("comment") == 0 || (*iter)->get_name().compare("small") == 0) {
		// 	continue;
		// }

		// for (int i = 0; i < depth; ++i) 
		// 	std::cerr << '\t';
		// std::cerr << (*iter)->get_name() << std::endl;
		// if((*iter)->get_name().compare("script") != 0) {

		// 	continue;
		// }
		if((*iter)->get_name().compare("script") == 0) {
			continue;
		}

		// Recursive
		if((*iter)->get_name().compare("text") != 0) {
			std::string inner = get_inner_text((*iter), depth + 1);
			if (!is_dead_string(inner)) {
				ret += inner;
			}
			continue;
		}

		const xmlpp::TextNode* text = dynamic_cast<const xmlpp::TextNode*>(*iter);

		if(!text) continue;

		std::string go = text->get_content();

		// This function just skips completely blank entries (with only spaces, or only \n or whatever
		// I'd remove this if you were to use it
		if(is_dead_string(go)) {
			continue;
		}

		std::string::size_type pos = 0;

		// Replaces newlines with spaces
		while((pos = go.find("\n", pos)) != std::string::npos) {
			go.replace(pos, 1, "\x20");
			pos++;
		}

		go.erase(std::remove_if(go.begin(), go.end(), is_punctuation), go.end());
		strip(go);
		if (go.length() > 0) {
			ret += go + "\n";
		}
	}

	return ret;
}

std::string get_inner_text(std::string html) {
	std::string tidy = html_xml(html);

	xmlpp::DomParser parser;
	parser.set_substitute_entities();
	parser.parse_memory(tidy);

	if(!parser) {
		throw std::logic_error("Parser problem.");
	}

	xmlpp::Document* document = parser.get_document();

	if(!document) {
		throw std::logic_error("Invalid Document.");
	}

	xmlpp::Node* root = document->get_root_node();

	if(!root) {
		throw std::logic_error("Invalid Document Root.");
	}
	return get_inner_text(root);
}

#endif // HTML_UTILS_HPP