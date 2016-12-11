#include<unistd.h>
#include"utils.h"
#include"path.h"
#include"except.h"

path getExecRoot(){
    char exePath[4096];
    memzero(exePath);
    ssize_t length = readlink("/proc/self/exe", exePath, sizeof(exePath));

    if (length == -1){
        throw syscall_error() << syscall_info("readlink", 10);
    }

    return path(exePath).parent_path();
}