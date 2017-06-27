#pragma once

#include<boost/format.hpp>
#include<string>
#include<cctype>

#include"utils.h"
#include"except.h"

#define FsmStart(state_t, state, chr_t, chr, workingBuffer, workingSize, workingIndex, backBuffer, absPtr, savepointPtr) \
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
    unsigned int* fsmAbsPtr = (absPtr);\
    unsigned int* fsmSavepointPtr = (savepointPtr);\
    (void)fsmAbsPtr;\
    (void)fsmSavepointPtr;\
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
    fsmBackBuffer.clear();\
    if (fsmAbsPtr != NULL){\
		*fsmAbsPtr = 0;\
	}

#define SaveStartSkip()\
    memset(fsmWorkingBuffer, 0, fsmWorkingIdx);\
    fsmWorkingIdx = 0;\
    fsmBackBuffer.clear();\
    if (*absPtr != NULL){\
		*absPtr = 0;\
	}

#define SaveThis()\
    if (fsmWorkingIdx == fsmWorkingSize){\
        fsmBackBuffer.append(fsmWorkingIdx, fsmWorkingSize);\
        memset(fsmWorkingBuffer, 0, fsmWorkingSize);\
        fsmWorkingIdx = 0;\
    }\
    fsmWorkingBuffer[fsmWorkingIdx++] = fsmChr;\
	if (fsmAbsPtr != NULL){\
		(*fsmAbsPtr)++;\
	}

#define SaveThisIfSame()\
    if (stCurrent == stNext){\
        SaveThis()\
    }

#define SaveStore(dest)\
    (dest).assign(fsmBackBuffer + std::string(fsmWorkingBuffer, fsmWorkingIdx));
#define SaveStoreOne(dest)\
    if (fsmWorkingIdx != 0){\
        SaveStore(dest)\
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

#define SavepointStore()\
	if (fsmSavepointPtr != NULL){\
		*fsmSavepointPtr = *fsmAbsPtr;\
	}

#define SavepointStoreOff(offset)\
	if (fsmSavepointPtr != NULL){\
		*fsmSavepointPtr = *fsmAbsPtr + (offset);\
	}

#define SavepointRevert()\
	if (fsmSavepointPtr != NULL && *fsmSavepointPtr != *fsmAbsPtr){\
		SaveBackspace(*fsmAbsPtr - *fsmSavepointPtr);\
		*fsmAbsPtr = *fsmSavepointPtr;\
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

#define TransIfWs(next, consume)\
	if (std::isspace(fsmChr)){\
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

#define TransElifWs(next, consume)\
	else if (std::isspace(fsmChr)){\
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
        BOOST_THROW_EXCEPTION(serverError() << stringInfo((boost::format("Bad state %1%!") % stNext).str()));\
    }
