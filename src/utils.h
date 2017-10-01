#pragma once
#include <string>
#include <ctime>
#include <boost/optional.hpp>
#include <boost/utility/string_ref.hpp>


#define memzero(buffer) memset(&buffer, 0, sizeof(buffer))

struct PipePair
{
	int readHead;
	int writeHead;
};

bool fdClosed(int fd);
std::string readFromFile(const std::string& filename);
std::string unixTimeToString(std::time_t timeVal);
std::string generateTagFromStat(const std::string& filename);

std::string randomAlpha(size_t size);

boost::optional<std::string> htmlEscapeRef(boost::string_ref htmlCode);

inline boost::optional<std::string> htmlEscapeRef(const std::string& htmlCode) {
	return htmlEscapeRef(boost::string_ref(htmlCode));
}
