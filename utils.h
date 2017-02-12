#pragma once

#define memzero(buffer) memset(&buffer, 0, sizeof(buffer))

struct PipePair {
	int readHead;
	int writeHead;
};

bool fdClosed(int fd);