#include<fstream>
#include<sstream>
#include<locale>
#include<boost/date_time/posix_time/posix_time.hpp>
#include<boost/date_time/local_time_adjustor.hpp>
#include<boost/date_time/c_local_time_adjustor.hpp>
#include <boost/python.hpp>
#include<poll.h>
#include<errno.h>
#include<sys/stat.h>
#include"utils.h"
#include"except.h"

#define DBG_DISABLE
#include "dbg.h"


namespace b = boost;
namespace bp = boost::python;


bool fdClosed(int fd) {
	char buffer[1024];

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int pollResult = poll(&pfd, 1, 0);
	while (pollResult != 0) {
		if (pollResult == -1) {
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("poll(): on fd %1% (to check if closed)", fd) << errcodeInfoDef());
		}
		int readResult = read(fd, buffer, 1024);
		if (readResult == -1) {
			if (errno == EBADF) {
				return true;
			}
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("read(): on fd %1% (to check if closed)", fd) << errcodeInfoDef());
		}
		if (readResult == 0) {
			return true;
		}
		pollResult = poll(&pfd, 1, 0);
	}

	return false;
}

errcodeInfo errcodeInfoDef() {
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

std::string pyErrAsString() {
	DBG("in errAsString()");
	if (PyErr_Occurred() == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Tried to call pyErrAsString() when no Python exception occured."));
	}

	try {
		PyObject *exception, *value, *traceback;
		bp::object formatted_list;
		PyErr_Fetch(&exception, &value, &traceback);

		bp::handle<> handleException(exception);
		bp::handle<> handleValue(bp::allow_null(value));
		bp::handle<> handleTraceback(bp::allow_null(traceback));

		bp::object moduleTraceback(bp::import("traceback"));

		DBG("In errAsString: gathered info");

		if (!traceback) {
			bp::object format_exception_only(moduleTraceback.attr("format_exception_only"));
			formatted_list = format_exception_only(handleException, handleValue);
		}
		else {
			bp::object format_exception(moduleTraceback.attr("format_exception"));
			formatted_list = format_exception(handleException, handleValue, handleTraceback);
		}

		DBG("In errAsString: gathered list");

		bp::object formatted = bp::str("\n").join(formatted_list);

		DBG("In errAsString: got formatted str");

		PyErr_Clear();

		DBG("Error cleared");

		if (formatted.is_none()) {
			return "Error info is None";
		}
		return bp::extract<std::string>(formatted);
	}
	catch (bp::error_already_set const&) {
		return "Error trying to get Python error info.";
	}
}

pyErrorInfo getPyErrorInfo() {
	return pyErrorInfo(pyErrAsString());
}

std::string unixTimeToString(std::time_t timeVal) {
	b::posix_time::ptime asPtime = b::posix_time::from_time_t(timeVal);

	std::ostringstream result;

	static char const* const fmt = "%a, %d %b %Y %H:%M:%S GMT";
	std::locale outLocale(std::locale::classic(), new b::posix_time::time_facet(fmt));
	result.imbue(outLocale);

	result << asPtime;

	return result.str();
}

std::string generateTagFromStat(std::string filename) {
	struct stat statResult;
	if (stat(filename.c_str(), &statResult) != 0) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("stat(): generating ETag") << errcodeInfoDef());
	}

	return formatString("%x%x%x", (int)statResult.st_ino, (int)statResult.st_size, (int)statResult.st_mtime);
}
