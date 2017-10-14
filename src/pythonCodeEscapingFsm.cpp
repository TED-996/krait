#include "pythonCodeEscapingFsm.h"


// MUST be kept in sync with the states enum in PythonCodeEscapingFsm::init.
static PythonCodeEscapingFsm::PythonCodeEscapingState escapingStateByFsmState[] = {
	//inStatement = 0,
	PythonCodeEscapingFsm::PythonCodeEscapingState::Statement,
	//inSqString,
	PythonCodeEscapingFsm::PythonCodeEscapingState::String,
	//inSqStringEscaping,
	PythonCodeEscapingFsm::PythonCodeEscapingState::String,
	//inDqString,
	PythonCodeEscapingFsm::PythonCodeEscapingState::String,
	//inDqStringEscaping,
	PythonCodeEscapingFsm::PythonCodeEscapingState::String,
	//inNullSqString,
	PythonCodeEscapingFsm::PythonCodeEscapingState::String,
	//inNullDqString,
	PythonCodeEscapingFsm::PythonCodeEscapingState::String,
	//afterNullSqString,
	PythonCodeEscapingFsm::PythonCodeEscapingState::Statement,
	//afterNullDqString,
	PythonCodeEscapingFsm::PythonCodeEscapingState::Statement,
	//inTripleSqString,
	PythonCodeEscapingFsm::PythonCodeEscapingState::MultilineString,
	//inTripleDqString,
	PythonCodeEscapingFsm::PythonCodeEscapingState::MultilineString,
	//inTripleSqStringEscaping,
	PythonCodeEscapingFsm::PythonCodeEscapingState::MultilineString,
	//inTripleDqStringEscaping,
	PythonCodeEscapingFsm::PythonCodeEscapingState::MultilineString,
	//inTripleSqStringOneSq,
	PythonCodeEscapingFsm::PythonCodeEscapingState::MultilineString,
	//inTripleSqStringTwoSq,
	PythonCodeEscapingFsm::PythonCodeEscapingState::MultilineString,
	//inTripleDqStringOneDq,
	PythonCodeEscapingFsm::PythonCodeEscapingState::MultilineString,
	//inTripleDqStringTwoDq,
	PythonCodeEscapingFsm::PythonCodeEscapingState::MultilineString,
	//inLineComment
	PythonCodeEscapingFsm::PythonCodeEscapingState::Comment
};

static char quoteChar[] = {
	//inStatement = 0,
	'\0',
	//inSqString,
	'\'',
	//inSqStringEscaping,
	'\'',
	//inDqString,
	'\"',
	//inDqStringEscaping,
	'\"',
	//inNullSqString,
	'\'',
	//inNullDqString,
	'\"',
	//afterNullSqString,
	'\0',
	//afterNullDqString,
	'\0',
	//inTripleSqString,
	'\'',
	//inTripleDqString,
	'\"',
	//inTripleSqStringEscaping,
	'\'',
	//inTripleDqStringEscaping,
	'\"',
	//inTripleSqStringOneSq,
	'\'',
	//inTripleSqStringTwoSq,
	'\'',
	//inTripleDqStringOneDq,
	'\"',
	//inTripleDqStringTwoDq,
	'\"',
	//inLineComment
	'#',
};



PythonCodeEscapingFsm::PythonCodeEscapingFsm() : FsmV2(20, 0, true) {
	init();
}

void PythonCodeEscapingFsm::init() {
	typedef SimpleFsmTransition Simple;
	typedef AlwaysFsmTransition Always;

	// MUST be kept in sync with the global array above
	enum {
		inStatement = 0,
		inSqString,
		inSqStringEscaping,
		inDqString,
		inDqStringEscaping,
		inNullSqString,
		inNullDqString,
		afterNullSqString,
		afterNullDqString,
		inTripleSqString,
		inTripleDqString,
		inTripleSqStringEscaping,
		inTripleDqStringEscaping,
		inTripleSqStringOneSq,
		inTripleSqStringTwoSq,
		inTripleDqStringOneDq,
		inTripleDqStringTwoDq,
		inLineComment
	};

	// Statements
	add(inStatement, new Simple('\'', inNullSqString));
	add(inStatement, new Simple('\"', inNullDqString));
	add(inStatement, new Simple('#', inLineComment));
	add(inStatement, new Always(inStatement));

	// Comments
	add(inLineComment, new Simple('\r', inStatement));
	add(inLineComment, new Simple('\n', inStatement));
	add(inLineComment, new Always(inLineComment));

	// Single quotes
	add(inNullSqString, new Simple('\\', inSqStringEscaping));
	add(inNullSqString, new Simple('\'', afterNullSqString));
	add(inNullSqString, new Always(inSqString));

	add(inSqStringEscaping, new Always(inSqString));

	add(inSqString, new Simple('\\', inSqStringEscaping));
	add(inSqString, new Simple('\'', inStatement));
	add(inSqString, new Always(inSqString));

	add(afterNullSqString, new Simple('\'', inTripleSqString));
	add(afterNullSqString, new Always(inStatement));

	// Triple single quotes
	add(inTripleSqString, new Simple('\\', inTripleSqStringEscaping));
	add(inTripleSqString, new Simple('\'', inTripleSqStringOneSq));
	add(inTripleSqString, new Always(inTripleSqString));

	add(inTripleSqStringEscaping, new Always(inTripleSqString));

	add(inTripleSqStringOneSq, new Simple('\\', inTripleSqStringEscaping));
	add(inTripleSqStringOneSq, new Simple('\'', inTripleSqStringTwoSq));
	add(inTripleSqStringOneSq, new Always(inTripleSqString));

	add(inTripleSqStringTwoSq, new Simple('\\', inTripleSqStringEscaping));
	add(inTripleSqStringTwoSq, new Simple('\'', inStatement));
	add(inTripleSqStringTwoSq, new Always(inTripleSqString));

	// Double quotes
	add(inNullDqString, new Simple('\\', inDqStringEscaping));
	add(inNullDqString, new Simple('\'', afterNullDqString));
	add(inNullDqString, new Always(inDqString));

	add(inDqStringEscaping, new Always(inDqString));

	add(inDqString, new Simple('\\', inDqStringEscaping));
	add(inDqString, new Simple('\'', inStatement));
	add(inDqString, new Always(inDqString));
	add(inDqString, new Always(inDqString));

	add(afterNullDqString, new Simple('\'', inTripleDqString));
	add(afterNullDqString, new Always(inStatement));

	// Triple double quotes
	add(inTripleDqString, new Simple('\\', inTripleDqStringEscaping));
	add(inTripleDqString, new Simple('\'', inTripleDqStringOneDq));
	add(inTripleDqString, new Always(inTripleDqString));

	add(inTripleDqStringEscaping, new Always(inTripleDqString));

	add(inTripleDqStringOneDq, new Simple('\\', inTripleDqStringEscaping));
	add(inTripleDqStringOneDq, new Simple('\'', inTripleDqStringTwoDq));
	add(inTripleDqStringOneDq, new Always(inTripleDqString));

	add(inTripleDqStringTwoDq, new Simple('\\', inTripleDqStringEscaping));
	add(inTripleDqStringTwoDq, new Simple('\'', inStatement));
	add(inTripleDqStringTwoDq, new Always(inTripleDqString));
}

PythonCodeEscapingFsm::PythonCodeEscapingState PythonCodeEscapingFsm::getCodeState() {
	return escapingStateByFsmState[getState()];
}

char PythonCodeEscapingFsm::getMultilineQuoteChar() {
	if (!isMultilineString()) {
		return '\0';
	}
	return quoteChar[getState()];
}
