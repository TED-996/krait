#pragma once
#include <string>
#include <vector>
#include <boost/python/object.hpp>
#include <boost/variant.hpp>
#include "IPymlCache.h"


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
		return NULL;
	}

	virtual const std::string* getEmbeddedString(std::string* storage) const override {
		return NULL;
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
};


class PymlItemSeq : public PymlItem
{
	std::vector<std::unique_ptr<const IPymlItem>> items;
public:
	PymlItemSeq(const std::vector<std::unique_ptr<const IPymlItem>>& items){
		for (const auto& it : items) {
			this->items.push_back(std::move(it));
		}
	}

	std::string runPyml() const override;
	bool isDynamic() const override;
	const IPymlItem* getNext(const IPymlItem* last) const override;
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
		if (last == NULL) {
			runPyml();
		}
		return NULL;
	}
};


class PymlItemIf : public PymlItem
{
	std::string conditionCode;
	std::unique_ptr<const IPymlItem> itemIfTrue;
	std::unique_ptr<const IPymlItem> itemIfFalse;

public:
	PymlItemIf(const std::string& conditionCode, std::unique_ptr<const IPymlItem> itemIfTrue, std::unique_ptr<const IPymlItem> itemIfFalse)
		: conditionCode(conditionCode), itemIfTrue(std::move(itemIfTrue)), itemIfFalse(std::move(itemIfFalse)) {
	}

	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const IPymlItem* getNext(const IPymlItem* last) const override;
};


class PymlItemFor : public PymlItem
{
	std::string initCode;
	std::string conditionCode;
	std::string updateCode;

	std::unique_ptr<const IPymlItem> loopItem;

public:
	PymlItemFor(std::string initCode, std::string conditionCode, std::string updateCode, std::unique_ptr<const IPymlItem> loopItem)
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
		if (last == NULL) {
			runPyml();
		}
		return NULL;
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
	PymlWorkingItem(Type type);

	template<typename T>
	T* getData() {
		return boost::get<T>(&data);
	}

	std::unique_ptr<const IPymlItem> getItem() const;
};
