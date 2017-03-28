#pragma once
#include<functional>
#include<vector>
#include<memory>
#include<queue>
#include<map>

class FsmTransition;

//FSM stands for Fancy State Machine.
class FsmV2 {
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
	std::vector<std::vector<std::shared_ptr<FsmTransition>>> transitions;
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

	static const size_t workingBufferSize = 1024;
	size_t workingIdx;
	char workingBuffer[workingBufferSize];
	std::string backBuffer;
	size_t savepointOffset;

	void store(char chr);

	void addToBulk(size_t state, FsmTransition* transition);
	void addStateActionToBulk(size_t state, fsmAction action);

	void execStateAction();

public:
	FsmV2(size_t nrStates, size_t nrBulkStates);

	void reset();

	size_t getState(){
		return state;
	}

	void add(size_t state, FsmTransition* transition);
	void addMany(std::initializer_list<std::pair<size_t, FsmTransition*>> transitions);
	void addStateAction(size_t state, fsmAction action);
	void addFinalAction(size_t state, fsmAction action);
	void addFinalActionToMany(fsmAction action, std::initializer_list<size_t> destinationStates);

	void addBulkParser(size_t startState, size_t endState, size_t failState, std::string strToMatch);
	void addStringLiteralParser(size_t startState, size_t endState, char delimiter, char escapeChr);
	void addBlockParser(size_t startState, size_t endState, char blockStart, char blockEnd);

	void consumeOne(char chr);
	void doFinalPass();

	std::string getStored();
	void resetStored();
	std::string getResetStored();

	void setSavepoint(size_t offset);
	void revertSavepoint();
	void setSkip(bool doSkip){
		skipThis = doSkip;
	}

	bool getIsFinalPass(){
		return isFinalPass;
	}

	void pushResetStoredString();
	std::string popStoredString();

	int getProp(std::string key);
	void setProp(std::string key, int value);
};

class FsmTransition {
public:
	virtual bool isMatch(char chr, FsmV2& fsm) = 0;
	virtual size_t getNextState(FsmV2 &fsm) = 0;
	virtual bool isConsume(FsmV2& fsm){
		return true;
	}
	virtual void execute(FsmV2& fsm){
	}
};


class ErrorFsmTransition : public FsmTransition {
public:
	bool isMatch(char chr, FsmV2& fsm) override;
	size_t getNextState(FsmV2 &fsm) override{
		return 0;
	}
};

class AlwaysFsmTransition : public FsmTransition {
private:
	size_t nextState;
	bool consume;
public:
	AlwaysFsmTransition(size_t nextState, bool consume=true);

	bool isMatch(char chr, FsmV2 &fsm) override {
		return true;
	}

	size_t getNextState(FsmV2 &fsm) override {
		return nextState;
	}

	bool isConsume(FsmV2 &fsm) override {
		return consume;
	}
};



class SimpleFsmTransition : public FsmTransition {
private:
	char chrToMatch;
	size_t nextState;
	bool consume;
public:
	SimpleFsmTransition(char chrToMatch, size_t nextState, bool consume=true);

	bool isMatch(char chr, FsmV2& fsm) override{
		return chr == chrToMatch;
	}
	size_t getNextState(FsmV2 &fsm) override{
		return nextState;
	}
	bool isConsume(FsmV2& fsm) override{
		return consume;
	}
};

class WhitespaceFsmTransition : public FsmTransition {
private:
	size_t nextState;
	bool consume;
public:
	WhitespaceFsmTransition(size_t nextState, bool consume=true);

	bool isMatch(char chr, FsmV2& fsm) override{
		return chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t';
	}
	size_t getNextState(FsmV2 &fsm) override{
		return nextState;
	}
	bool isConsume(FsmV2& fsm) override{
		return consume;
	}
};

class ActionFsmTransition : public FsmTransition {
private:
	FsmV2::fsmAction action;
	std::shared_ptr<FsmTransition> transition;
public:
	ActionFsmTransition(FsmTransition* transition, FsmV2::fsmAction action)
			: action(action), transition(transition){
	}
	bool isMatch(char chr, FsmV2& fsm) override {
		return transition->isMatch(chr, fsm);
	}
	size_t getNextState(FsmV2 &fsm) override {
		return transition->getNextState(fsm);
	}
	bool isConsume(FsmV2& fsm) override {
		return transition->isConsume(fsm);
	}
	void execute(FsmV2& fsm) override {
		transition->execute(fsm);
		action(fsm);
	}
};

class SavepointSetFsmTransition : public FsmTransition {
private:
	size_t offset;
	std::shared_ptr<FsmTransition> transition;
public:
	SavepointSetFsmTransition(FsmTransition* transition, size_t offset = 0)
		: offset(offset), transition(transition){
	}
	bool isMatch(char chr, FsmV2& fsm) override {
		return transition->isMatch(chr, fsm);
	}
	size_t getNextState(FsmV2 &fsm) override {
		return transition->getNextState(fsm);
	}
	bool isConsume(FsmV2& fsm) override {
		return transition->isConsume(fsm);
	}
	void execute(FsmV2& fsm) override {
		transition->execute(fsm);
		fsm.setSavepoint(offset);
	}
};

class SavepointRevertFsmTransition : public FsmTransition {
private:
	std::shared_ptr<FsmTransition> transition;
public:
	SavepointRevertFsmTransition(FsmTransition* transition)
		: transition(transition){
	}
	bool isMatch(char chr, FsmV2& fsm) override {
		return transition->isMatch(chr, fsm);
	}
	size_t getNextState(FsmV2 &fsm) override {
		return transition->getNextState(fsm);
	}
	bool isConsume(FsmV2& fsm) override {
		return transition->isConsume(fsm);
	}
	void execute(FsmV2& fsm) override {
		transition->execute(fsm);
		fsm.revertSavepoint();
	}
};

class DiscardFsmTransition : public FsmTransition {
private:
	std::shared_ptr<FsmTransition> transition;
public:
	DiscardFsmTransition(FsmTransition* transition)
	: transition(transition){
	}
	bool isMatch(char chr, FsmV2& fsm) override {
		return transition->isMatch(chr, fsm);
	}
	size_t getNextState(FsmV2 &fsm) override {
		return transition->getNextState(fsm);
	}
	bool isConsume(FsmV2& fsm) override {
		return transition->isConsume(fsm);
	}
	void execute(FsmV2& fsm) override {
		transition->execute(fsm);
		fsm.resetStored();
	}
};

class PushFsmTransition : public FsmTransition {
private:
	std::shared_ptr<FsmTransition> transition;
public:
	PushFsmTransition(FsmTransition* transition)
		: transition(transition){
	}
	bool isMatch(char chr, FsmV2& fsm) override {
		return transition->isMatch(chr, fsm);
	}
	size_t getNextState(FsmV2 &fsm) override {
		return transition->getNextState(fsm);
	}
	bool isConsume(FsmV2& fsm) override {
		return transition->isConsume(fsm);
	}
	void execute(FsmV2& fsm) override {
		transition->execute(fsm);
		fsm.pushResetStoredString();
	}
};


class SkipFsmTransition : public FsmTransition {
private:
	std::shared_ptr<FsmTransition> transition;
public:
	SkipFsmTransition(FsmTransition* transition)
	: transition(transition){
	}
	bool isMatch(char chr, FsmV2& fsm) override {
		return transition->isMatch(chr, fsm);
	}
	size_t getNextState(FsmV2 &fsm) override {
		return transition->getNextState(fsm);
	}
	bool isConsume(FsmV2& fsm) override {
		return transition->isConsume(fsm);
	}
	void execute(FsmV2& fsm) override {
		transition->execute(fsm);
		fsm.setSkip(true);
	}
};


class FinalFsmTransition : public FsmTransition {
	size_t finalState;
public:
	FinalFsmTransition(size_t finalState)
			: finalState(finalState){
	}
	bool isMatch(char chr, FsmV2 &fsm) override {
		return fsm.getIsFinalPass();
	}

	size_t getNextState(FsmV2 &fsm) override {
		return finalState;
	}
};

class OrFinalFsmTransition : public FsmTransition {
private:
	std::shared_ptr<FsmTransition> transition;
public:
	OrFinalFsmTransition(FsmTransition* transition)
	: transition(transition){
	}
	bool isMatch(char chr, FsmV2& fsm) override {
		return fsm.getIsFinalPass() || transition->isMatch(chr, fsm);
	}
	size_t getNextState(FsmV2 &fsm) override {
		return transition->getNextState(fsm);
	}
	bool isConsume(FsmV2& fsm) override {
		return transition->isConsume(fsm);
	}
	void execute(FsmV2& fsm) override {
		transition->execute(fsm);
	}
};

class AndConditionFsmTransition : public FsmTransition {
private:
	std::shared_ptr<FsmTransition> transition;
	std::function<bool(char, FsmV2&)> condition;
public:
	AndConditionFsmTransition(FsmTransition* transition, std::function<bool(char, FsmV2&)> condition)
	: transition(transition), condition(condition){
	}
	bool isMatch(char chr, FsmV2& fsm) override {
		return condition(chr, fsm) && transition->isMatch(chr, fsm);
	}
	size_t getNextState(FsmV2 &fsm) override {
		return transition->getNextState(fsm);
	}
	bool isConsume(FsmV2& fsm) override {
		return transition->isConsume(fsm);
	}
	void execute(FsmV2& fsm) override {
		transition->execute(fsm);
	}
};