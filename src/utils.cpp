#include <fstream>
#include <sstream>
#include <locale>
#include <random>
#include <chrono>
#include <ctime>
#include <boost/python.hpp>
#include <poll.h>
#include <errno.h>
#include <sys/stat.h>
#include "utils.h"
#include "except.h"
#include "pythonModule.h"

//#define DBG_DISABLE
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
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("read(): on fd %1% (to check if closed)", fd) << errcodeInfoDef());
			}
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

std::string readFromFile(const std::string& filename) {
	std::ifstream fileIn(filename, std::ios::in | std::ios::binary);

	if (!fileIn) {
		fileIn.close();
		BOOST_THROW_EXCEPTION(notFoundError() << stringInfoFromFormat("Error: File not found: %1%", filename));
	}

	std::ostringstream fileData;
	fileData << fileIn.rdbuf();
	fileIn.close();

	return fileData.str();
}

std::string pyErrAsString() {
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

		if (!traceback) {
			bp::object format_exception_only(moduleTraceback.attr("format_exception_only"));
			formatted_list = format_exception_only(handleException, handleValue);
		}
		else {
			bp::object format_exception(moduleTraceback.attr("format_exception"));
			formatted_list = format_exception(handleException, handleValue, handleTraceback);
		}

		bp::str formatted = bp::str("\n").join(formatted_list);

		PyErr_Clear();

		return bp::extract<std::string>(formatted);
	}
	catch (bp::error_already_set const&) {
		return "Error trying to get Python error info.";
	}
}

pyErrorInfo getPyErrorInfo() {
	return pyErrorInfo(pyErrAsString());
}

pythonError getPythonError() {
	pythonError result;
	
	if (PyErr_Occurred() == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Tried to call pyErrAsString() when no Python exception occured."));
	}
	PyObject *exception = nullptr;
	PyObject *value = nullptr;
	PyObject *traceback = nullptr;

	try {

		PyErr_Fetch(&exception, &value, &traceback);

		bp::handle<> handleException(exception);
		bp::handle<> handleValue(bp::allow_null(value));
		bp::handle<> handleTraceback(bp::allow_null(traceback));

		result << pyExcTypeInfo(bp::object(handleException));

		bp::object tracebackModule(bp::import("traceback"));
		bp::object formattedList;

		if (!traceback) {
			bp::object format_exception_only(tracebackModule.attr("format_exception_only"));
			formattedList = format_exception_only(handleException, handleValue);
		}
		else {
			bp::object format_exception(tracebackModule.attr("format_exception"));
			formattedList = format_exception(handleException, handleValue, handleTraceback);
		}

		bp::str formatted = bp::str("\n").join(formattedList);

		result << pyErrorInfo(bp::extract<std::string>(formatted));
	}
	catch (bp::error_already_set const&) {
		result << pyErrorInfo("<Error trying to get Python error info>");
	}
	return result;
}

std::string unixTimeToString(std::time_t timeVal) {
	static const char* fmt = "%a, %d %b %Y %H:%M:%S GMT";

	struct tm* asGmt = std::gmtime(&timeVal);
	char result[40];
	size_t len = std::strftime(result, 40, fmt, asGmt);

	if (len == 0) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("unixTimeToString: strftime failed."));
	}

	return std::string(result, len);
}

std::string generateTagFromStat(const std::string& filename) {
	struct stat statResult;
	if (stat(filename.c_str(), &statResult) != 0) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("stat(): generating ETag") << errcodeInfoDef());
	}

	return formatString("%x%x%x", (int)statResult.st_ino, (int)statResult.st_size, (int)statResult.st_mtime);
}

std::string randomAlpha(size_t size) {
	static const char values[] =
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> distribution(0, size - 1);

	std::string result;
	result.reserve(size);
	for (size_t i = 0; i < size; i++) {
		result[i] = values[distribution(generator)];
	}

	return result;
}

std::vector<std::string> getHtmlReplacements() {
	std::vector<std::string> replacements(256);
	replacements[(size_t)'&'] = "&amp;";
	replacements[(size_t)'<'] = "&lt;";
	replacements[(size_t)'>'] = "&gt;";
	replacements[(size_t)'"'] = "&quot;";
	replacements[(size_t)'\''] = "&apos;";
	return replacements;
}

boost::optional<std::string> htmlEscapeRef(boost::string_ref htmlCode) {
	std::string result;
	bool resultEmpty = true;

	static std::vector<std::string> replacements = getHtmlReplacements();

	unsigned int oldIdx = 0;
	for (unsigned int idx = 0; idx < htmlCode.length(); idx++) {
		if (replacements[(int)htmlCode[idx]].size() != 0) {
			if (resultEmpty) {
				result.reserve(htmlCode.length() + htmlCode.length() / 10); //Approximately...
				resultEmpty = false;
			}

			result.append(htmlCode.data() + oldIdx, idx - oldIdx);
			result.append(replacements[(int)htmlCode[idx]]);
			oldIdx = idx + 1;
		}
	}

	if (resultEmpty) {
		return boost::none;
	}
	else {
		result.append(htmlCode.data() + oldIdx, htmlCode.length() - oldIdx);
		return result;
	}
}

std::vector<std::string> getReprReplacements() {
	std::vector<std::string> replacements(256);

	for (int i = 0; i <= 0x1f; i++) {
		replacements[i] = formatString("\\x%02X", i);
	}
	for (int i = 0x7f; i < 0x100; i++) {
		replacements[i] = formatString("\\x%02X", i);
	}

	replacements[(size_t)'\r'] = "\\r";
	replacements[(size_t)'\n'] = "\\n";
	replacements[(size_t)'\t'] = "\\t";
	replacements[(size_t)'\\'] = "\\\\";
	replacements[(size_t)'\''] = "\\'";

	return replacements;
}

std::string reprPythonString(const std::string& str) {
	std::string result;
	result.reserve(str.length() + 2); //The bare minimum.
	result.append(1, '\'');

	static std::vector<std::string> replacements = getReprReplacements();

	unsigned int oldIdx = 0;
	for (unsigned int idx = 0; idx < str.length(); idx++) {
		unsigned char ch = (unsigned char)str[idx];
		if (replacements[ch].size() != 0) {
			result.append(str.data() + oldIdx, idx - oldIdx);
			result.append(replacements[ch]);
			oldIdx = idx + 1;
		}
	}

	result.append(str.data() + oldIdx, str.length() - oldIdx);
	result.append(1, '\'');

	return result;
}
