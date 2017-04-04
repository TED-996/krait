#pragma once
#include<string>
#include<vector>
#include<boost/pool/object_pool.hpp>
#include<boost/variant.hpp>
#include"IPymlCache.h"


class PymlItem : public IPymlItem {
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


class PymlItemStr : public PymlItem {
	std::string str;
public:
	PymlItemStr(const std::string& str){
		this->str = str;
	}


	std::string runPyml() const override {
		return str;
	}

	bool isDynamic() const override {
		return false;
	}

	const std::string* getEmbeddedString(std::string* storage) const override{
		return &str;
	}
};


class PymlItemSeq : public PymlItem {
	std::vector<const PymlItem*> items;
public:
	PymlItemSeq(const std::vector<const PymlItem*>& items){
		this->items = items;
	}

	std::string runPyml() const override;
	bool isDynamic() const override;

	const PymlItem* tryCollapse() const;

	const IPymlItem* getNext(const IPymlItem* last) const override;
};


class PymlItemPyEval : public PymlItem {
	std::string code;
public:
	PymlItemPyEval(const std::string& code){
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


class PymlItemPyEvalRaw : public PymlItem {
	std::string code;
public:
	PymlItemPyEvalRaw(const std::string& code){
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


class PymlItemPyExec : public PymlItem {
	std::string code;
public:
	PymlItemPyExec(const std::string& code){
		this->code = code;
	}
	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const IPymlItem* getNext(const IPymlItem* last) const override{
		if (last == NULL){
			runPyml();
		}
		return NULL;
	}
};


class PymlItemIf : public PymlItem {
	std::string conditionCode;
	const PymlItem* itemIfTrue;
	const PymlItem* itemIfFalse;

public:
	PymlItemIf(const std::string& conditionCode, const PymlItem* itemIfTrue, const PymlItem* itemIfFalse){
		this->conditionCode = conditionCode;
		this->itemIfTrue = itemIfTrue;
		this->itemIfFalse = itemIfFalse;
	}

	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const IPymlItem* getNext(const IPymlItem* last) const override;
};


class PymlItemFor : public PymlItem {
	std::string initCode;
	std::string conditionCode;
	std::string updateCode;

	const PymlItem* loopItem;

public:
	PymlItemFor(const std::string& initCode, const std::string& conditionCode, const std::string& updateCode, const PymlItem* loopItem){
		this->initCode = initCode;
		this->conditionCode = conditionCode;
		this->updateCode = updateCode;
		this->loopItem = loopItem;
	}

	std::string runPyml() const override;

	bool isDynamic() const override {
		return true;
	}

	const IPymlItem* getNext(const IPymlItem* last) const override;
};


class PymlItemEmbed : public PymlItem {
private:
	std::string filename;
	IPymlCache* cache;

public:
	PymlItemEmbed(std::string filename, IPymlCache& cache)
			: filename(filename), cache(&cache){
	}

	std::string runPyml() const override;

	const IPymlItem* getNext(const IPymlItem* last) const override;

	bool isDynamic() const override;
};


struct PymlItemPool {
	boost::object_pool<PymlItem> itemPool;
	boost::object_pool<PymlItemStr> strPool;
	boost::object_pool<PymlItemSeq> seqPool;
	boost::object_pool<PymlItemPyEval> pyEvalPool;
	boost::object_pool<PymlItemPyEvalRaw> pyEvalRawPool;
	boost::object_pool<PymlItemPyExec> pyExecPool;
	boost::object_pool<PymlItemIf> ifExecPool;
	boost::object_pool<PymlItemFor> forExecPool;
	boost::object_pool<PymlItemEmbed> embedPool;
};


struct PymlWorkingItem {
	enum Type { None, Str, Seq, PyEval, PyEvalRaw, PyExec, If, For, Embed };
	struct NoneData {
	};
	struct StrData {
		std::string str;
	};
	struct SeqData {
		std::vector<PymlWorkingItem*> items;
	};
	struct PyCodeData {
		Type type;
		std::string code;
	};
	struct IfData {
		std::string condition;
		PymlWorkingItem* itemIfTrue;
		PymlWorkingItem* itemIfFalse;
	};
	struct ForData {
		std::string initCode;
		std::string conditionCode;
		std::string updateCode;
		PymlWorkingItem* loopItem;
	};
	struct EmbedData {
		std::string filename;
		IPymlCache* cache;
	};

	boost::variant<NoneData, StrData, SeqData, PyCodeData, IfData, ForData, EmbedData> data;

	Type type;
	PymlWorkingItem(Type type);

	template<typename T>
	T* getData(){
		return boost::get<T>(&data);
	}

	const PymlItem* getItem(PymlItemPool& pool) const;

};

