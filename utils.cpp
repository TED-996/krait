#include<poll.h>
#include<errno.h>
#include"utils.h"
#include"except.h"

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