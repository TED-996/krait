#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "except.h"
#include "path.h"
#include "utils.h"

namespace bf = boost::filesystem;

bf::path getExecRoot() {
    char exePath[4096];
    memzero(exePath);
    ssize_t length = readlink("/proc/self/exe", exePath, sizeof(exePath));

    if (length == -1) {
        BOOST_THROW_EXCEPTION(syscallError() << stringInfo("readlink(): getting exec root") << errcodeInfoDef());
    }

    return bf::path(exePath).parent_path();
}

bf::path getShareRoot() {
    bf::path base = getExecRoot();
    if (!base.has_parent_path()) {
        return base / "share" / "krait";
    }
    return base.parent_path() / "share" / "krait";
}

bool pathCheckExists(std::string filename) {
    struct stat data;
    int stat_result = stat(filename.c_str(), &data);
    if (stat_result == 0) {
        return true;
    } else {
        BOOST_THROW_EXCEPTION(
            syscallError() << stringInfoFromFormat("not found: by stat on %1%", filename.c_str()) << errcodeInfoDef());
    }
}
