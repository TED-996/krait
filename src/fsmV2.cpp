#include "fsmV2.h"
#include "except.h"

using namespace std;

SimpleFsmTransition::SimpleFsmTransition(char chrToMatch, size_t nextState, bool consume)
	: chrToMatch(chrToMatch),
	nextState(nextState),
	consume(consume) {
}

WhitespaceFsmTransition::WhitespaceFsmTransition(size_t nextState, bool consume)
	: nextState(nextState),
	consume(consume){
}

AlwaysFsmTransition::AlwaysFsmTransition(size_t nextState, bool consume) : nextState(nextState), consume(consume) {
}

bool ErrorFsmTransition::isMatch(char chr, FsmV2& fsm) {
	BOOST_THROW_EXCEPTION(serverError() << stringInfo("Invalid FSM state"));
}


FsmV2::FsmV2(size_t nrStates, size_t nrBulkStates) :
	maxState(nrStates),
	maxBulkState(nrStates + nrBulkStates),
	currBulkState(nrStates) {
	transitions.resize(nrStates + nrBulkStates);
	stateActions.resize(nrStates + nrBulkStates);
	finalActions.resize(nrStates);
	bulkFailState.resize(nrBulkStates);

	reset();
}

void FsmV2::reset() {
	while(!storedStrings.empty()){
		storedStrings.pop();
	}
	parserProps.clear();
	state = 0;
	skipThis = false;
	workingIdx = 0;
	backBuffer.clear();
	savepointOffset = 0;
	isFinalPass = false;
}

void FsmV2::add(size_t state, FsmTransition *transition) {
	if (state >= maxState){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Requested state to add transition to larger than maximum."));
	}

	transitions[state].emplace_back(transition);
}

void FsmV2::addMany(std::initializer_list<std::pair<size_t, FsmTransition *>> transitions) {
	for (const auto& transition : transitions){
		add(transition.first, transition.second);
	}
}

void FsmV2::addStateAction(size_t state, FsmV2::fsmAction action) {
	if (state >= maxState){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Requested state to add action to larger than maximum."));
	}

	stateActions[state].push_back(action);
}

void FsmV2::addFinalAction(size_t state, FsmV2::fsmAction action) {
	if (state >= maxState){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Requested state to add final action to larger than maximum."));
	}

	finalActions[state].push_back(action);
}

void FsmV2::addFinalActionToMany(fsmAction action, std::initializer_list<size_t> destinationStates) {
	for (auto state : destinationStates){
		if (state >= maxState){
			BOOST_THROW_EXCEPTION(serverError() << stringInfo("Requested state to add final action to larger than maximum."));
		}
		finalActions[state].push_back(action);
	}
}

void FsmV2::addToBulk(size_t state, FsmTransition *transition) {
	if (state >= maxBulkState){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Requested bulk state to add transition to larger than maximum."));
	}

	transitions[maxState + state].emplace_back(transition);
}

void FsmV2::addStateActionToBulk(size_t state, FsmV2::fsmAction action) {
	if (state >= maxBulkState){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Requested state to add action to larger than maximum."));
	}

	stateActions[maxState + state].push_back(action);
}

void FsmV2::setSavepoint(size_t offset) {
	savepointOffset = offset;
}

void FsmV2::revertSavepoint() {
	if (savepointOffset <= workingIdx){
		workingIdx -= savepointOffset;
	}
	else{
		savepointOffset -= workingIdx;
		workingIdx = 0;

		backBuffer.erase(backBuffer.length() - savepointOffset);
	}
}

void FsmV2::pushResetStoredString() {
	storedStrings.push(getResetStored());
}

std::string FsmV2::popStoredString() {
	string result(std::move(storedStrings.front()));
	storedStrings.pop();
	return result;
}

int FsmV2::getProp(std::string key) {
	const auto it = parserProps.find(key);
	if (it == parserProps.end()){
		return 0;
	}
	else{
		return it->second;
	}
}

void FsmV2::setProp(std::string key, int value) {
	parserProps[key] = value;
}

void FsmV2::store(char chr) {
	if (workingIdx == workingBufferSize){
		backBuffer.append(workingBuffer, workingIdx);
		workingIdx = 0;
	}
	workingBuffer[workingIdx++] = chr;
	savepointOffset++;
}

std::string FsmV2::getStored() {
	std::string result = backBuffer;
	result.append(workingBuffer, workingIdx);

	return result;
}

void FsmV2::resetStored() {
	workingIdx = 0;
	backBuffer.clear();
}

std::string FsmV2::getResetStored() {
	std::string result = std::move(backBuffer);
	result.append(workingBuffer, workingIdx);

	workingIdx = 0;
	backBuffer.clear();

	return result;
}

void FsmV2::addBulkParser(size_t startState, size_t endState, size_t failState, std::string strToMatch) {
	if (currBulkState + strToMatch.length() - 1 >= maxBulkState) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("FSM error: Out of bulk states."));
	}
	if (strToMatch.length() == 0){
		add(startState, new AlwaysFsmTransition(endState, false));
		return;
	}

	if (strToMatch.length() == 1){
		add(startState, new SimpleFsmTransition(strToMatch[0], endState));
		add(startState, new AlwaysFsmTransition(failState, false));
	}

	add(startState, new SimpleFsmTransition(strToMatch[0], currBulkState));

	for (string::iterator it = strToMatch.begin() + 1; it + 1 < strToMatch.end(); it++) {
		addToBulk(currBulkState, new SimpleFsmTransition(*it, currBulkState + 1));
		addToBulk(currBulkState, new AlwaysFsmTransition(failState, false));
		bulkFailState[currBulkState - maxState] = failState;
		currBulkState++;
	}
	addToBulk(currBulkState, new SimpleFsmTransition(strToMatch[strToMatch.length() - 1], endState));
	addToBulk(currBulkState, new AlwaysFsmTransition(failState, false));
	bulkFailState[currBulkState - maxState] = failState;
	currBulkState++;
}

void FsmV2::addStringLiteralParser(size_t startState, size_t endState, char delimiter, char escapeChr) {
	if (currBulkState + 2 >= maxBulkState){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("FSM error: Out of bulk states."));
	}

	size_t consumeState = currBulkState;
	size_t escapeState = currBulkState + 1;
	currBulkState += 2;

	add(startState, new SimpleFsmTransition(delimiter, consumeState));
	addToBulk(consumeState, new SimpleFsmTransition(escapeChr, escapeState));
	addToBulk(escapeState, new AlwaysFsmTransition(consumeState));
	addToBulk(consumeState, new SimpleFsmTransition(delimiter, endState));
	addToBulk(consumeState, new AlwaysFsmTransition(consumeState));

	bulkFailState[consumeState - maxState] = startState;
	bulkFailState[escapeState - maxState] = startState;
}

void FsmV2::consumeOne(char chr) {
	execStateAction();

	bool consumed = false;
	skipThis = false;
	while(!consumed){
		consumed = true;
		size_t idx;
		size_t len = transitions[state].size();
		for (idx = 0; idx < len && !transitions[state][idx]->isMatch(chr, *this); idx++){
		}

		//if there was a match
		if (idx < len){
			const auto& found = transitions[state][idx];
			found->execute(*this);
			state = found->getNextState(*this);
			consumed = found->isConsume(*this);
		}
	}

	if (!skipThis && !isFinalPass){
		store(chr);
	}
}

void FsmV2::doFinalPass() {
	isFinalPass = true;
	consumeOne('\n');

	size_t actionState = state;
	if (state > maxState){
		actionState = bulkFailState[state - maxState];
	}

	for (const auto& action : finalActions[actionState]){
		action(*this);
	}
}


void FsmV2::execStateAction() {
	if (state >= maxBulkState){
		BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Invalid state in FSM: %1%", state));
	}
	for (const auto& it : stateActions[state]){
		it(*this);
	}
}
