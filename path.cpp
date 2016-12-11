#include<unistd.h>
#include"utils.h"
#include"path.h"
#include"except.h"

using namespace boost::filesystem;

path getExecRoot(){
    char exePath[4096];
    memzero(exePath);
    ssize_t length = readlink("/proc/self/exe", exePath, sizeof(exePath));

    if (length == -1){
        throw syscall_error() << string_info("readlink") << errcode_info(10);
    }

    return path(exePath).parent_path();
}