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
	virtual void addPymlWorkingStr(std::string&& str) = 0;
	virtual void addPymlWorkingPyCode(PymlWorkingItem::Type type, std::string&& code) = 0;
	virtual void addPymlWorkingEmbed(std::string&& filename) = 0;
	virtual void addPymlWorkingCtrl(std::string&& ctrlCode) = 0;
	virtual void pushPymlWorkingIf(std::string&& condition) = 0;
	virtual bool addSeqToPymlWorkingIf() = 0;
	virtual void pushPymlWorkingFor() = 0;
	virtual void addCodeToPymlWorkingFor(int where, std::string&& code) = 0;
	virtual bool addSeqToPymlWorkingFor() = 0;
	virtual void pushPymlWorkingForIn(std::string&& entry, std::string&& collection) = 0;
	virtual void pushPymlWorkingSeq() = 0;
	virtual void addPymlStackTop() = 0;
};


class V2PymlParserFsm : public FsmV2
{
private:
	class PymlRootTransition : public DelegatingFsmTransition
	{
	protected:
		IV2PymlParser** parser;
	public:
		PymlRootTransition(IV2PymlParser** parser, FsmTransition*&& delegate)
			: DelegatingFsmTransition(std::move(delegate)), parser(parser) {
		}
	};

	class PymlAddStrTransition : public PymlRootTransition
	{
	public:
		PymlAddStrTransition(IV2PymlParser** parser, FsmTransition*&& base)
			: PymlRootTransition(parser, std::move(base)) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);
			(*parser)->addPymlWorkingStr(fsm.getResetStored());
		}

		FsmTransition& getDelegateExecute() override{
			return *this;
		}
	};

	class PymlAddPyCodeTransition : public PymlRootTransition
	{
	private:
		PymlWorkingItem::Type type;
	public:
		PymlAddPyCodeTransition(IV2PymlParser** parser, FsmTransition*&& base,
		                        PymlWorkingItem::Type type)
			: PymlRootTransition(parser, std::move(base)), type(type) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);
			(*parser)->addPymlWorkingPyCode(type, fsm.getResetStored());
		}

		FsmTransition& getDelegateExecute() override {
			return *this;
		}
	};

	class PymlAddEmbedTransition : public PymlRootTransition
	{
	public:
		PymlAddEmbedTransition(IV2PymlParser** parser, FsmTransition*&& base)
			: PymlRootTransition(parser, std::move(base)) {
		}

	public:
		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->addPymlWorkingEmbed(fsm.getResetStored());
		}

		FsmTransition& getDelegateExecute() override {
			return *this;
		}
	};

	class PymlAddCtrlTransition : public PymlRootTransition
	{
	public:
		PymlAddCtrlTransition(IV2PymlParser** parser, FsmTransition*&& base)
			: PymlRootTransition(parser, std::move(base)) {
		}

	public:
		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->addPymlWorkingCtrl(fsm.getResetStored());
		}

		FsmTransition& getDelegateExecute() override {
			return *this;
		}
	};

	class PymlAddFor3Transition : public PymlRootTransition
	{
	public:
		PymlAddFor3Transition(IV2PymlParser** parser, FsmTransition*&& base)
			: PymlRootTransition(parser, std::move(base)) {
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

		FsmTransition& getDelegateExecute() override {
			return *this;
		}
	};

	class PymlAddForInTransition : public PymlRootTransition
	{
	public:
		PymlAddForInTransition(IV2PymlParser** parser, FsmTransition*&& base)
			: PymlRootTransition(parser, std::move(base)) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);
			std::string entry = fsm.popStoredString();
			std::string collection = fsm.popStoredString();
			(*parser)->pushPymlWorkingForIn(std::move(entry), std::move(collection));
			(*parser)->pushPymlWorkingSeq();

			fsm.resetStored();
		}

		FsmTransition& getDelegateExecute() override {
			return *this;
		}
	};

	class PymlAddIfTransition : public PymlRootTransition
	{
	public:
		PymlAddIfTransition(IV2PymlParser** parser, FsmTransition*&& base)
			: PymlRootTransition(parser, std::move(base)) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->pushPymlWorkingIf(fsm.getResetStored());
			(*parser)->pushPymlWorkingSeq();

			fsm.resetStored();
		}

		FsmTransition& getDelegateExecute() override {
			return *this;
		}
	};

	class PymlAddElseTransition : public PymlRootTransition
	{
	public:
		PymlAddElseTransition(IV2PymlParser** parser, FsmTransition*&& base)
			: PymlRootTransition(parser, std::move(base)) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->addSeqToPymlWorkingIf();
			(*parser)->pushPymlWorkingSeq();
			fsm.resetStored();
		}

		FsmTransition& getDelegateExecute() override {
			return *this;
		}
	};

	class PymlFinishForTransition : public PymlRootTransition
	{
	public:
		PymlFinishForTransition(IV2PymlParser** parser, FsmTransition*&& base)
			: PymlRootTransition(parser, std::move(base)) {
		}

		void execute(FsmV2& fsm) override {
			PymlRootTransition::execute(fsm);

			(*parser)->addSeqToPymlWorkingFor();
			(*parser)->addPymlStackTop();
			fsm.resetStored();
		}

		FsmTransition& getDelegateExecute() override {
			return *this;
		}
	};

	class PymlFinishIfTransition : public PymlRootTransition
	{
	public:
		PymlFinishIfTransition(IV2PymlParser** parser, FsmTransition*&& base)
			: PymlRootTransition(parser, std::move(base)) {
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

		FsmTransition& getDelegateExecute() override {
			return *this;
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
	boost::object_pool<PymlWorkingItem> workingItemPool;

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
	void addPymlWorkingStr(std::string&& str) override;
	void addPymlWorkingPyCode(PymlWorkingItem::Type type, std::string&& code) override;
	void addPymlWorkingEmbed(std::string&& filename) override;
	void addPymlWorkingCtrl(std::string&& ctrlCode) override;
	void pushPymlWorkingIf(std::string&& condition) override;
	bool addSeqToPymlWorkingIf() override;
	void pushPymlWorkingFor() override;
	void addCodeToPymlWorkingFor(int where, std::string&& code) override;
	bool addSeqToPymlWorkingFor() override;
	void pushPymlWorkingForIn(std::string&& entry, std::string&& collection) override;
	void pushPymlWorkingSeq() override;
	void addPymlStackTop() override;

	V2PymlParser(IPymlCache& cache);

	void consume(std::string::iterator start, std::string::iterator end) override;
	std::unique_ptr<const IPymlItem> getParsed() override;
};
