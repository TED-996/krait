#pragma once
#include <string>
#include <vector>
#include <boost/python/object.hpp>
#include <boost/variant.hpp>
#include "IPymlCache.h"
#include "except.h"


class PymlItem : public IPymlItem
{
public:
	virtual std::string runPyml() const override {
		return "";
	}

	virtual bool isDynamic() const override {
		return false;
	}

	virtual const IPymlItem* getNext(const IPymlItem* last) const override {
		return nullptr;
	}

	virtual const std::string* getEmbeddedString(std::string* storage) const override {
		return nullptr;
	}

	bool canConvertToCode() const override {
		return true;
	}

	CodeAstItem getCodeAst() const override {
		return CodeAstItem("pass"); //TODO: shouldn't. Maybe raise exception?
	}

	std::unique_ptr<CodeAstItem> getHeaderAst() const override {
		return nullptr;
	}
};


class PymlItemStr : public PymlItem
{
	std::string str;
public:
	PymlItemStr(const std::string& str) {
		this->str = str;
	}


	std::string runPyml() const override {
		return str;
	}

	bool isDynamic() const override {
		return false;
	}

	const std::string* getEmbeddedString(std::string* storage) const override {
		return &str;
	}

	CodeAstItem getCodeAst() const override;
};


class PymlItemSeq : public PymlItem
{
	std::vector<std::unique_ptr<const IPymlItem>> items;
public:
	PymlItemSeq(std::vector<std::unique_ptr<const IPymlItem>>&& items)
		: items(std::move(items)) {
	}

	std::string runPyml() const override;
	bool isDynamic() const override;
	const IPymlItem* getNext(const IPymlItem* last) const override;

	CodeAstItem getCodeAst() const override;
	bool canConvertToCode() const override;
	std::unique_ptr<CodeAstItem> getHeaderAst() const override;
};


class PymlItemPyEval : public PymlItem
{
	std::string code;
public:
	PymlItemPyEval(const std::string& code) {
		this->code = code;
	}

	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const std::string* getEmbeddedString(std::string* storage) const override {
		storage->assign(runPyml());
		return storage;
	}

	CodeAstItem getCodeAst() const override;
};


class PymlItemPyEvalRaw : public PymlItem
{
	std::string code;
public:
	PymlItemPyEvalRaw(const std::string& code) {
		this->code = code;
	}

	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const std::string* getEmbeddedString(std::string* storage) const override {
		storage->assign(runPyml());
		return storage;
	}

	CodeAstItem getCodeAst() const override;
};


class PymlItemPyExec : public PymlItem
{
	std::string code;
public:
	PymlItemPyExec(const std::string& code) {
		this->code = code;
	}

	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const IPymlItem* getNext(const IPymlItem* last) const override {
		if (last == nullptr) {
			runPyml();
		}
		return nullptr;
	}

	CodeAstItem getCodeAst() const override;
};


class PymlItemIf : public PymlItem
{
	std::string conditionCode;
	std::unique_ptr<const IPymlItem> itemIfTrue;
	std::unique_ptr<const IPymlItem> itemIfFalse;

public:
	PymlItemIf(const std::string& conditionCode, std::unique_ptr<const IPymlItem>&& itemIfTrue, std::unique_ptr<const IPymlItem>&& itemIfFalse)
		: conditionCode(conditionCode), itemIfTrue(std::move(itemIfTrue)), itemIfFalse(std::move(itemIfFalse)) {
	}

	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const IPymlItem* getNext(const IPymlItem* last) const override;
	CodeAstItem getCodeAst() const override;
	bool canConvertToCode() const override;
	std::unique_ptr<CodeAstItem> getHeaderAst() const override;
};


class PymlItemFor : public PymlItem
{
	std::string initCode;
	std::string conditionCode;
	std::string updateCode;

	std::unique_ptr<const IPymlItem> loopItem;

public:
	PymlItemFor(std::string initCode, std::string conditionCode, std::string updateCode, std::unique_ptr<const IPymlItem>&& loopItem)
		: initCode(initCode),
		  conditionCode(conditionCode),
		  updateCode(updateCode),
		  loopItem(std::move(loopItem)) {
	}

	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const IPymlItem* getNext(const IPymlItem* last) const override;
	CodeAstItem getCodeAst() const override;
	bool canConvertToCode() const override;
	std::unique_ptr<CodeAstItem> getHeaderAst() const override;
};


class PymlItemEmbed : public PymlItem
{
private:
	std::string filename;
	IPymlCache& cache;

public:
	PymlItemEmbed(std::string filename, IPymlCache& cache)
		: filename(filename), cache(cache) {
	}

	std::string runPyml() const override;

	const IPymlItem* getNext(const IPymlItem* last) const override;

	bool isDynamic() const override;
	CodeAstItem getCodeAst() const override;
	std::unique_ptr<CodeAstItem> getHeaderAst() const override;
};

class PymlItemSetCallable : public PymlItem
{
private:
	const boost::python::object& callable;
	std::string destination;

public:

	PymlItemSetCallable(const boost::python::object& callable, const std::string& destination)
		: callable(callable),
		  destination(destination) {
	}

	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const IPymlItem* getNext(const IPymlItem* last) const override {
		if (last == nullptr) {
			runPyml();
		}
		return nullptr;
	}
	CodeAstItem getCodeAst() const override {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("PymlItemSetCallable::getCodeAst() called without canConvertToCode being called."));
	}
	bool canConvertToCode() const override {
		return false;
	}
};

struct PymlWorkingItem
{
	enum Type
	{
		None,
		Str,
		Seq,
		PyEval,
		PyEvalRaw,
		PyExec,
		If,
		For,
		Embed
	};

	struct NoneData
	{
	};

	struct StrData
	{
		std::string str;
	};

	struct SeqData
	{
		std::vector<PymlWorkingItem*> items;
	};

	struct PyCodeData
	{
		Type type;
		std::string code;
	};

	struct IfData
	{
		std::string condition;
		PymlWorkingItem* itemIfTrue;
		PymlWorkingItem* itemIfFalse;
	};

	struct ForData
	{
		std::string initCode;
		std::string conditionCode;
		std::string updateCode;
		PymlWorkingItem* loopItem;
	};

	struct EmbedData
	{
		std::string filename;
		IPymlCache* cache;
	};

	boost::variant<NoneData, StrData, SeqData, PyCodeData, IfData, ForData, EmbedData> data;

	Type type;
	explicit PymlWorkingItem(Type type);

	template<typename T>
	T* getData() {
		return boost::get<T>(&data);
	}

	std::unique_ptr<const IPymlItem> getItem() const;
};
