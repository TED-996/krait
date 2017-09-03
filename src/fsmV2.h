#pragma once
#include<functional>
#include<vector>
#include<memory>
#include<queue>
#include<map>

class FsmTransition;

//FSM stands for Fancy State Machine.
class FsmV2
{
public:
	typedef std::function<void(FsmV2&)> fsmAction;
private:
	/*State lifecycle:
		* skipThis = false;
		* stateActions[state]()
		* in transitions[state] search forward until transitions[state][i].isMatch()
		* transitions[state][i].execute()
		* nextState = transitions[state][i].getNextState();
		* consume = transitions[state][i].isConsume();
		* state = nextState;
		* if not consume: repeat
		* if not skipThis: store
	 */
	std::vector<std::vector<std::unique_ptr<FsmTransition>>> transitions;
	std::vector<std::vector<fsmAction>> stateActions;
	std::vector<std::vector<fsmAction>> finalActions;

	std::vector<size_t> bulkFailState;

	std::queue<std::string> storedStrings;
	std::map<std::string, int> parserProps;

	size_t maxState;
	size_t maxBulkState;

	size_t currBulkState;

	size_t state;
	bool skipThis;

	bool isFinalPass;

	bool hasStateActions;

	static const size_t workingBufferSize = 1024;
	size_t workingIdx;
	char workingBuffer[workingBufferSize];
	std::string backBuffer;
	size_t savepointOffset;

	void store(char chr);

	void addToBulk(size_t state, FsmTransition*&& transition);
	void addStateActionToBulk(size_t state, const fsmAction& action);

	void execStateAction();

public:
	FsmV2(size_t nrStates, size_t nrBulkStates);
	virtual ~FsmV2() = default;

	void reset();

	size_t getState() {
		return state;
	}

	void add(size_t state, FsmTransition*&& transition);
	void addMany(std::initializer_list<std::pair<size_t, FsmTransition*&&>> transitions);
	void addStateAction(size_t state, const fsmAction& action);
	void addFinalAction(size_t state, const fsmAction& action);
	void addFinalActionToMany(const fsmAction& action, std::initializer_list<size_t> destinationStates);

	void addBulkParser(size_t startState, size_t endState, size_t failState, const std::string& strToMatch);
	void addStringLiteralParser(size_t startState, size_t endState, char delimiter, char escapeChr);
	void addBlockParser(size_t startState, size_t endState, char blockStart, char blockEnd);

	void consumeOne(char chr);
	void doFinalPass(char consumeChr);

	std::string getStored();
	void resetStored();
	std::string getResetStored();

	void setSavepoint(size_t offset);
	void revertSavepoint();

	void setSkip(bool doSkip) {
		skipThis = doSkip;
	}

	bool getIsFinalPass() {
		return isFinalPass;
	}

	void pushResetStoredString();
	std::string popStoredString();

	int getProp(const std::string& key);
	void setProp(const std::string& key, int value);
};

class FsmTransition
{
public:
	virtual ~FsmTransition() = default;
	virtual bool isMatch(char chr, FsmV2& fsm) = 0;
	virtual size_t getNextState(FsmV2& fsm) = 0;

	virtual bool isConsume(FsmV2& fsm) {
		return true;
	}

	virtual void execute(FsmV2& fsm) {
	}

	virtual FsmTransition& getDelegateIsMatch() {
		return *this;
	}
	virtual FsmTransition& getDelegateNextState() {
		return *this;
	}
	virtual FsmTransition& getDelegateIsConsume() {
		return *this;
	}
	virtual FsmTransition& getDelegateExecute() {
		return *this;
	}
};


class ErrorFsmTransition : public FsmTransition
{
public:
	bool isMatch(char chr, FsmV2& fsm) override;

	size_t getNextState(FsmV2& fsm) override {
		return 0;
	}
};

class AlwaysFsmTransition : public FsmTransition
{
private:
	size_t nextState;
	bool consume;
public:
	AlwaysFsmTransition(size_t nextState, bool consume = true);

	bool isMatch(char chr, FsmV2& fsm) override {
		return true;
	}

	size_t getNextState(FsmV2& fsm) override {
		return nextState;
	}

	bool isConsume(FsmV2& fsm) override {
		return consume;
	}
};


class SimpleFsmTransition : public FsmTransition
{
private:
	char chrToMatch;
	size_t nextState;
	bool consume;
public:
	SimpleFsmTransition(char chrToMatch, size_t nextState, bool consume = true);

	bool isMatch(char chr, FsmV2& fsm) override {
		return chr == chrToMatch;
	}

	size_t getNextState(FsmV2& fsm) override {
		return nextState;
	}

	bool isConsume(FsmV2& fsm) override {
		return consume;
	}
};

class WhitespaceFsmTransition : public FsmTransition
{
private:
	size_t nextState;
	bool consume;
public:
	WhitespaceFsmTransition(size_t nextState, bool consume = true);

	bool isMatch(char chr, FsmV2& fsm) override {
		return chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t';
	}

	size_t getNextState(FsmV2& fsm) override {
		return nextState;
	}

	bool isConsume(FsmV2& fsm) override {
		return consume;
	}
};

class DelegatingFsmTransition : public FsmTransition
{
private:
	std::unique_ptr<FsmTransition> delegate;

	FsmTransition& isMatchDelegate;
	FsmTransition& nextStateDelegate;
	FsmTransition& isConsumeDelegate;
	FsmTransition& executeDelegate;
public:
	explicit DelegatingFsmTransition(FsmTransition*&& delegate) :
		delegate(delegate), 
		isMatchDelegate(this->delegate->getDelegateIsMatch()),
		nextStateDelegate(this->delegate->getDelegateNextState()),
		isConsumeDelegate(this->delegate->getDelegateIsConsume()),
		executeDelegate(this->delegate->getDelegateExecute()){
	}

	bool isMatch(char chr, FsmV2& fsm) override {
		return isMatchDelegate.isMatch(chr, fsm);
	}
	size_t getNextState(FsmV2& fsm) override {
		return nextStateDelegate.getNextState(fsm);
	}
	bool isConsume(FsmV2& fsm) override {
		return isConsumeDelegate.isConsume(fsm);
	}
	void execute(FsmV2& fsm) override {
		executeDelegate.execute(fsm);
	}

	FsmTransition& getDelegateIsMatch() override{
		return isMatchDelegate;
	}
	FsmTransition& getDelegateNextState() override{
		return nextStateDelegate;
	}
	FsmTransition& getDelegateIsConsume() override{
		return isConsumeDelegate;
	}
	FsmTransition& getDelegateExecute() override{
		return executeDelegate;
	}
};

class ActionFsmTransition : public DelegatingFsmTransition
{
private:
	FsmV2::fsmAction action;
public:
	ActionFsmTransition(FsmTransition*&& delegate, FsmV2::fsmAction action)
		: DelegatingFsmTransition(std::move(delegate)), action(action) {
	}

	void execute(FsmV2& fsm) override {
		DelegatingFsmTransition::execute(fsm);
		action(fsm);
	}

	FsmTransition& getDelegateExecute() override{
		return *this;
	}
};

class SavepointSetFsmTransition : public DelegatingFsmTransition
{
private:
	size_t offset;
public:
	SavepointSetFsmTransition(FsmTransition*&& delegate, size_t offset = 0)
		: DelegatingFsmTransition(std::move(delegate)), offset(offset) {
	}

	void execute(FsmV2& fsm) override {
		DelegatingFsmTransition::execute(fsm);
		fsm.setSavepoint(offset);
	}

	FsmTransition& getDelegateExecute() override {
		return *this;
	}
};

class SavepointRevertFsmTransition : public DelegatingFsmTransition
{
public:
	SavepointRevertFsmTransition(FsmTransition*&& delegate)
		: DelegatingFsmTransition(std::move(delegate)) {
	}

	void execute(FsmV2& fsm) override {
		DelegatingFsmTransition::execute(fsm);
		fsm.revertSavepoint();
	}

	FsmTransition& getDelegateExecute() override{
		return *this;
	}
};

class DiscardFsmTransition : public DelegatingFsmTransition
{
public:
	DiscardFsmTransition(FsmTransition*&& delegate)
		: DelegatingFsmTransition(std::move(delegate)) {
	}

	void execute(FsmV2& fsm) override {
		DelegatingFsmTransition::execute(fsm);
		fsm.resetStored();
	}

	FsmTransition& getDelegateExecute() override{
		return *this;
	}
};

class PushFsmTransition : public DelegatingFsmTransition
{
public:
	PushFsmTransition(FsmTransition*&& delegate)
		: DelegatingFsmTransition(std::move(delegate)) {
	}

	void execute(FsmV2& fsm) override {
		DelegatingFsmTransition::execute(fsm);
		fsm.pushResetStoredString();
	}

	FsmTransition& getDelegateExecute() override{
		return *this;
	}
};


class SkipFsmTransition : public DelegatingFsmTransition
{
public:
	SkipFsmTransition(FsmTransition*&& delegate)
		: DelegatingFsmTransition(std::move(delegate)) {
	}

	void execute(FsmV2& fsm) override {
		DelegatingFsmTransition::execute(fsm);
		fsm.setSkip(true);
	}

	FsmTransition& getDelegateExecute() override {
		return *this;
	}
};


class FinalFsmTransition : public FsmTransition
{
	size_t finalState;
public:
	FinalFsmTransition(size_t finalState)
		: finalState(finalState) {
	}

	bool isMatch(char chr, FsmV2& fsm) override {
		return fsm.getIsFinalPass();
	}

	size_t getNextState(FsmV2& fsm) override {
		return finalState;
	}
};

class OrFinalFsmTransition : public DelegatingFsmTransition
{
public:
	OrFinalFsmTransition(FsmTransition*&& delegate)
		: DelegatingFsmTransition(std::move(delegate)){
	}

	bool isMatch(char chr, FsmV2& fsm) override {
		return fsm.getIsFinalPass() || DelegatingFsmTransition::isMatch(chr, fsm);
	}

	FsmTransition& getDelegateIsMatch() override{
		return *this;
	}
};

class AndConditionFsmTransition : public DelegatingFsmTransition
{
private:
	std::function<bool(char, FsmV2&)> condition;
public:
	AndConditionFsmTransition(FsmTransition*&& delegate, std::function<bool(char, FsmV2&)> condition)
		: DelegatingFsmTransition(std::move(delegate)), condition(condition) {
	}

	bool isMatch(char chr, FsmV2& fsm) override {
		return condition(chr, fsm) && DelegatingFsmTransition::isMatch(chr, fsm);
	}

	FsmTransition& getDelegateIsMatch() override {
		return *this;
	}
};
