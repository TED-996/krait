#pragma once

#include<boost/format.hpp>
#include<string>

#include"utils.h"
#include"except.h"
#include"dbg.h"

#define FsmStart(state_t, state, chr_t, chr, workingBuffer, workingSize, workingIndex, backBuffer) \
    state_t stCurrent = (state);\
    state_t stNext = (state);\
    state_t* statePtr = (state_t*)&(state);\
    unsigned int* fsmWorkingIdxPtr = (unsigned int*)&(workingIndex);\
    chr_t fsmChr = chr;\
    bool retVal = true;\
    chr_t* fsmWorkingBuffer = (workingBuffer);\
    unsigned int fsmWorkingSize = (workingSize);\
    unsigned int fsmWorkingIdx = (unsigned int)(workingIndex);\
    std::string& fsmBackBuffer(backBuffer);\
    switch(stCurrent){

#define FsmEnd()\
	    default: \
	        throw httpParseError() << stringInfo((boost::format("Bad state %1%!")% state).str());\
	}\
	*statePtr = stNext;\
	*fsmWorkingIdxPtr = fsmWorkingIdx;\
	return retVal;


#define StatesBegin(value)\
    case value:

#define StatesNext(value)\
	    break;\
	case value:

#define StatesEnd()\
    break;


#define SaveStart()\
    memset(fsmWorkingBuffer, 0, fsmWorkingIdx);\
    fsmWorkingBuffer[0] = fsmChr;\
    fsmWorkingIdx = 1;\
    fsmBackBuffer.clear();

#define SaveStartSkip()\
    memset(fsmWorkingBuffer, 0, fsmWorkingIdx);\
    fsmWorkingIdx = 0;\
    fsmBackBuffer.clear();

#define SaveThis()\
    if (fsmWorkingIdx == fsmWorkingSize){\
        fsmBackBuffer.append(fsmWorkingIdx, fsmWorkingSize);\
        memset(fsmWorkingBuffer, 0, fsmWorkingSize);\
        fsmWorkingIdx = 0;\
    }\
    fsmWorkingBuffer[fsmWorkingIdx++] = fsmChr;

#define SaveThisIfSame()\
    if (stCurrent == stNext){\
        if (fsmWorkingIdx == fsmWorkingSize){\
            fsmBackBuffer.append(fsmWorkingIdx, fsmWorkingSize);\
            memset(fsmWorkingBuffer, 0, fsmWorkingSize);\
            fsmWorkingIdx = 0;\
            }\
        fsmWorkingBuffer[fsmWorkingIdx++] = fsmChr;\
    }

#define SaveStore(dest)\
    (dest).assign(fsmBackBuffer + std::string(fsmWorkingBuffer, fsmWorkingIdx));\

#define SaveStoreOne(dest)\
    if (fsmWorkingIdx != 0){\
        (dest).assign(fsmBackBuffer + std::string(fsmWorkingBuffer, fsmWorkingIdx));\
    }

#define SaveBackspace(chars){\
	unsigned int charsLeft = (chars);\
	if (fsmWorkingIdx >= charsLeft){\
		memset(fsmWorkingBuffer + fsmWorkingIdx - charsLeft, 0, chars);\
		fsmWorkingIdx -= charsLeft;\
	}\
	else{\
		memset(fsmWorkingBuffer, 0, fsmWorkingIdx);\
		charsLeft -= fsmWorkingIdx;\
		fsmWorkingIdx = 0;\
		if (fsmBackBuffer.size() >= charsLeft){\
			fsmBackBuffer.resize(fsmBackBuffer.size() - charsLeft);\
		}\
		else{\
			fsmBackBuffer.resize(0);\
		}\
	}\
}

#define SaveDiscard()\
    memset(fsmWorkingBuffer, 0, fsmWorkingIdx);\
    fsmWorkingIdx = 0;\
    fsmBackBuffer.clear();


#define TransIf(value, next, consume)\
    if (fsmChr == value){\
        stNext = (next);\
        retVal = (consume);\
    }

#define TransIfCond(value, cond, next, consume)\
    if (fsmChr == value && (cond)){\
        stNext = (next);\
        retVal = (consume);\
    }

#define TransIfOther(value, next, consume)\
    if (fsmChr != value){\
        stNext = (next);\
        retVal = (consume);\
    }

#define TransIfOtherCond(value, cond, next, consume)\
    if (fsmChr != value && (cond)){\
        stNext = (next);\
        retVal = (consume);\
    }

#define TransElif(value, next, consume)\
    else if (fsmChr == value){\
        stNext = (next);\
        retVal = (consume);\
    }

#define TransElifCond(value, cond, next, consume)\
    else if (fsmChr == value && (cond)){\
        stNext = (next);\
        retVal = (consume);\
    }

#define TransAlways(next, consume)\
    stNext = (next);\
    retVal = (consume);

#define TransElse(next, consume)\
    else {\
        stNext = (next);\
        retVal = (consume);\
    }

#define TransElseError()\
    else {\
        throw httpParseError() << stringInfo((boost::format("Bad state %1%!") % stNext).str());\
    }
