#pragma once
#include <stack>
#include <string>
#include <boost/pool/object_pool.hpp>
#include "pymlItems.h"
#include "IPymlCache.h"
#include "IPymlParser.h"
#include "fsmV2.h"

class IV2PymlParser : public IPymlParser
{
public:
	virtual void addPymlWorkingStr(const std::string& str) = 0;
	virtual void addPymlWorkingPyCode(PymlWorkingItem::Type type, const std::string& code) = 0;
	virtual void addPymlWorkingEmbed(const std::string& filename) = 0;
	virtual void addPymlWorkingCtrl(const std::string& ctrlCode) = 0;
	virtual void pushPymlWorkingIf(const std::string& condition) = 0;
	virtual bool addSeqToPymlWorkingIf() = 0;
	virtual void pushPymlWorkingFor() = 0;
	virtual void addCodeToPymlWorkingFor(int where, const std::string& code) = 0;
	virtual bool addSeqToPymlWorkingFor() = 0;
	virtual void pushPymlWorkingForIn(std::string entry, std::string collection) = 0;
	virtual void pushPymlWorkingSeq() = 0;
	virtual void addPymlStackTop() = 0;
};


class V2PymlParserFsm : public FsmV2
{
private:
	class PymlRootTransition : public FsmTransition
	{
	protected:
		IV2PymlParser** parser;
		std::unique_ptr<FsmTransition> base;
	public:
		PymlRootTransition(IV2PymlParser** parser, FsmTransition* base)
			: parser(parser), base(base) {
		}

		bool isMatch(char chr, FsmV2& fsm) override {
			return base->isMatch(chr, fsm);
		}

		size_t getNextState(FsmV2& fsm) override {
			return base->getNextState(fsm);
		}

		bool isConsume(FsmV2& fsm) override {
			return base->isConsume(fsm);
		}

		virtual void execute(FsmV2& fsm) override {
			base->execute(fsm);
		}
	};

	class PymlAddStrTransition : public PymlRootTransition
	{
	public:
		PymlAddStrTransition(IV2PymlParser** parser, FsmTransition* base)
			: PymlRootTransition(parser, base) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);
			(*parser)->addPymlWorkingStr(fsm.getResetStored());
		}
	};

	class PymlAddPyCodeTransition : public PymlRootTransition
	{
	private:
		PymlWorkingItem::Type type;
	public:
		PymlAddPyCodeTransition(IV2PymlParser** parser, FsmTransition* base,
		                        PymlWorkingItem::Type type)
			: PymlRootTransition(parser, base), type(type) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);
			(*parser)->addPymlWorkingPyCode(type, fsm.getResetStored());
		}
	};

	class PymlAddEmbedTransition : public PymlRootTransition
	{
	public:
		PymlAddEmbedTransition(IV2PymlParser** parser, FsmTransition* base)
			: PymlRootTransition(parser, base) {
		}

	public:
		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->addPymlWorkingEmbed(fsm.getResetStored());
		}
	};

	class PymlAddCtrlTransition : public PymlRootTransition
	{
	public:
		PymlAddCtrlTransition(IV2PymlParser** parser, FsmTransition* base)
			: PymlRootTransition(parser, base) {
		}

	public:
		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->addPymlWorkingCtrl(fsm.getResetStored());
		}
	};

	class PymlAddFor3Transition : public PymlRootTransition
	{
	public:
		PymlAddFor3Transition(IV2PymlParser** parser, FsmTransition* base)
			: PymlRootTransition(parser, base) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->pushPymlWorkingFor();
			(*parser)->addCodeToPymlWorkingFor(0, fsm.popStoredString());
			(*parser)->addCodeToPymlWorkingFor(1, fsm.popStoredString());
			(*parser)->addCodeToPymlWorkingFor(2, fsm.popStoredString());
			(*parser)->pushPymlWorkingSeq();

			fsm.resetStored();
		}
	};

	class PymlAddForInTransition : public PymlRootTransition
	{
	public:
		PymlAddForInTransition(IV2PymlParser** parser, FsmTransition* base)
			: PymlRootTransition(parser, base) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);
			std::string entry = fsm.popStoredString();
			std::string collection = fsm.popStoredString();
			(*parser)->pushPymlWorkingForIn(entry, collection);
			(*parser)->pushPymlWorkingSeq();

			fsm.resetStored();
		}
	};

	class PymlAddIfTransition : public PymlRootTransition
	{
	public:
		PymlAddIfTransition(IV2PymlParser** parser, FsmTransition* base)
			: PymlRootTransition(parser, base) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->pushPymlWorkingIf(fsm.getResetStored());
			(*parser)->pushPymlWorkingSeq();

			fsm.resetStored();
		}
	};

	class PymlAddElseTransition : public PymlRootTransition
	{
	public:
		PymlAddElseTransition(IV2PymlParser** parser, FsmTransition* base)
			: PymlRootTransition(parser, base) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->addSeqToPymlWorkingIf();
			(*parser)->pushPymlWorkingSeq();
			fsm.resetStored(); //TODO: add in other places maybe !!IMPORTANT
		}
	};

	class PymlFinishForTransition : public PymlRootTransition
	{
	public:
		PymlFinishForTransition(IV2PymlParser** parser, FsmTransition* base)
			: PymlRootTransition(
				parser, base) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->addSeqToPymlWorkingFor();
			(*parser)->addPymlStackTop();
			fsm.resetStored();
		}
	};

	class PymlFinishIfTransition : public PymlRootTransition
	{
	public:
		PymlFinishIfTransition(IV2PymlParser** parser, FsmTransition* base)
			: PymlRootTransition(parser, base) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			if (!(*parser)->addSeqToPymlWorkingIf()) {
				BOOST_THROW_EXCEPTION(
					pymlError() << stringInfo("Could not finish if instruction; are there 2 else branches?"));
			}
			(*parser)->addPymlStackTop();
			fsm.resetStored();
		}
	};


	IV2PymlParser* parser;

	void init();

	void finalAddPymlStr();
	void finalThrowError(const pymlError& err);
public:
	V2PymlParserFsm();

	void setParser(IV2PymlParser* parser) {
		this->parser = parser;
	}
};


class V2PymlParser : public IV2PymlParser
{
private:
	std::unique_ptr<const IPymlItem> rootItem;

	std::stack<PymlWorkingItem> itemStack;
	boost::object_pool<PymlWorkingItem> workingItemPool;  //TODO: Migrate to unique ptrs or something

	IPymlCache& cache;

	int krItIndex;

	template<typename T>
	bool stackTopIsType() {
		return !itemStack.empty() && boost::get<T>(&itemStack.top().data) != nullptr;
	}

	template<typename T>
	T& getStackTop() {
		return boost::get<T>(itemStack.top().data);
	}

	template<typename T>
	T& popStackTop() {
		T& result = boost::get<T>(itemStack.top().data);
		itemStack.pop();
		return result;
	}

	static V2PymlParserFsm parserFsm;

public:
	void addPymlWorkingStr(const std::string& str) override;
	void addPymlWorkingPyCode(PymlWorkingItem::Type type, const std::string& code) override;
	void addPymlWorkingEmbed(const std::string& filename) override;
	void addPymlWorkingCtrl(const std::string& ctrlCode) override;
	void pushPymlWorkingIf(const std::string& condition) override;
	bool addSeqToPymlWorkingIf() override;
	void pushPymlWorkingFor() override;
	void addCodeToPymlWorkingFor(int where, const std::string& code) override;
	bool addSeqToPymlWorkingFor() override;
	void pushPymlWorkingForIn(const std::string& entry, const std::string& collection) override;
	void pushPymlWorkingSeq() override;
	void addPymlStackTop() override;

	V2PymlParser(IPymlCache& cache);

	void consume(std::string::iterator start, std::string::iterator end) override;
	std::unique_ptr<const IPymlItem> getParsed() override;
};
