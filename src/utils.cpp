#include<fstream>
#include<sstream>
#include<locale>
#include<boost/date_time/posix_time/posix_time.hpp>
#include<boost/date_time/local_time_adjustor.hpp>
#include<boost/date_time/c_local_time_adjustor.hpp>
#include<poll.h>
#include<errno.h>
#include<sys/stat.h>
#include"utils.h"
#include"except.h"

#define DBG_DISABLE
#include "dbg.h"


namespace b = boost;


bool fdClosed(int fd){
	char buffer[1024];

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int pollResult = poll(&pfd, 1, 0);
	while(pollResult != 0){
		if(pollResult == -1){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("poll(): on fd %1% (to check if closed)", fd) << errcodeInfoDef());
		}
		int readResult = read(fd, buffer, 1024);
		if (readResult == -1){
			if (errno == EBADF){
				return true;
			}
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("read(): on fd %1% (to check if closed)", fd) << errcodeInfoDef());
		}
		if (readResult == 0){
			return true;
		}
		pollResult = poll(&pfd, 1, 0);
	}

	return false;
}

errcodeInfo errcodeInfoDef(){
	return errcodeInfo(errno);
}

std::string readFromFile(std::string filename) {
	std::ifstream fileIn(filename, std::ios::in | std::ios::binary);

	if (!fileIn) {
		DBG("except in readFromFile");
		fileIn.close();
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}

	std::ostringstream fileData;
	fileData << fileIn.rdbuf();
	fileIn.close();

	return fileData.str();
}

std::string unixTimeToString(std::time_t timeVal){
	b::posix_time::ptime asPtime = b::posix_time::from_time_t(timeVal);

	std::ostringstream result;	

	static char const* const fmt = "%a, %d %b %Y %H:%M:%S GMT";
	std::locale outLocale(std::locale::classic(), new b::posix_time::time_facet(fmt));
	result.imbue(outLocale);

	result << asPtime;

	return result.str();
}

void generateTagFromStat(std::string filename, char* dest){
	struct stat statResult;
	if (stat(filename.c_str(), &statResult) != 0){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("stat(): generating ETag") << errcodeInfoDef());
	}

	std::string result = formatString("%x%x%x", (int)statResult.st_ino, (int)statResult.st_size, (int)statResult.st_mtime);
	strcpy(dest, result.c_str());
}
