#include<boost/format.hpp>

#include"request.h"
#include"utils.h"
#include"except.h"

RequestParser::RequestParser() {
	state = 1;
	finished = false;
	memzero(workingStr);
	workingIdx = 0;
}

#define FsmStart(state)\
	int stNext = (state);\
	bool retVal = true;\
	switch(stNext){

#define FsmEnd(state)\
		default: \
			throw parseError() << stringInfo((boost::format("Bad state %1%!")% state).str());\
	}\
	state = stNext;\
	return retVal;
	
	
#define StatesBegin(value)\
		case value:

#define StatesNext(value)\
			break;\
		case value:

#define StatesEnd()\
			break;

			
#define SaveStart()\
			memset(workingStr, 0, workingIdx);\
			workingStr[workingIdx] = chr;\
			workingIdx = 1;

#define SaveStartSkip()\
			memset(workingStr, 0, workingIdx);\
			workingIdx = 0;

#define SaveThis()\
			if (workingIdx == sizeof(workingStr)){\
				workingBackBuffer.append(workingIdx, (unsigned int)sizeof(workingStr));\
				memzero(workingStr);\
				workingIdx = 0;\
			}\
			workingStr[workingIdx++] = chr;

#define SaveStore(dest)\
			(dest).assign(workingStr, workingIdx);

#define SaveStoreOne(dest)\
			if (workingIdx != 0){\
				(dest).assign(workingStr, workingIdx);\
			}


#define TransIf(value, next, consume)\
			if (chr == value){\
				stNext = (next);\
				retVal = (consume);\
			}

#define TransIfCond(value, cond, next, consume)\
			if (chr == value && (cond)){\
				stNext = (next);\
				retVal = (consume);\
			}

#define TransElif(value, next, consume)\
			else if (chr == value){\
				stNext = (next);\
				retVal = (consume);\
			}

#define TransElifCond(value, cond, next, consume)\
			else if (chr == value && (cond)){\
				stNext = (next);\
				retVal = (consume);\
			}

#define TransElse(next, consume)\
			else {\
				stNext = (next);\
				retVal = (consume);\
			}

#define TransElseError()\
			else {\
				throw parseError() << stringInfo((format("Bad state %1%!" % stNext).str()));\
			}


bool RequestParser::consumeOne ( char chr ) {
	FsmStart(state)
		StatesBegin(0)
			SaveStartSkip()
			TransIf(' ', 0, true)
			TransElse(1, true)
		StatesNext(1)
			SaveThis()
			TransIf(' ', 2, true)
			TransElse(1, true)
		StatesNext(2)
			SaveStoreOne(methodString)
			SaveStartSkip()
			TransIf(' ', 2, true)
			TransElse(3, true)
		StatesNext(3)
			SaveThis()
			TransIf(' ', 4, true)
			TransElse(3, true)
		StatesNext()
		StatesEnd()
			
	FsmEnd(state)
}

