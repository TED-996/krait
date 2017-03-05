#include<unistd.h>
#include<errno.h>

#include"utils.h"
#include"path.h"
#include"except.h"

using namespace boost::filesystem;

path getExecRoot() {
	char exePath[4096];
	memzero(exePath);
	ssize_t length = readlink("/proc/self/exe", exePath, sizeof(exePath));

	if (length == -1) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("readlink(): getting exec root") << errcodeInfoDef());
	}

	return path(exePath).parent_path();
}

path getCreateDataRoot(){
	return getExecRoot();
}