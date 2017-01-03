#pragma once
#include<string>
#include<vector>
#include<stack>
#include<boost/pool/object_pool.hpp>
#inlcude<boost/variant.hpp>


class PymlItem {
public:
	virtual std::string runPyml() const {
		return "";
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
};


class PymlItemSeq : public PymlItem {
	std::vector<const PymlItem*> items;
public:
	PymlItemSeq(const std::vector<const PymlItem*>& items){
		this->items = items;
	}
	
	std::string runPyml() const override;
	
	const PymlItem* tryCollapse() const;
};


class PymlItemPyEval : public PymlItem {
	std::string code;
public:
	PymlItemPyEval(const std::string& code){
		this->code = code;
	}
	std::string runPyml() const override;
};


class PymlItemPyEvalRaw : public PymlItem {
	std::string code;
public:
	PymlItemPyEvalRaw(const std::string& code){
		this->code = code;
	}
	std::string runPyml() const override;
};


class PymlItemPyExec : public PymlItem {
	std::string code;
public:
	PymlItemPyExec(const std::string& code){
		this->code = code;
	}
	std::string runPyml() const override;
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
};


struct PymlWorkingItem {
	enum Type { None, Str, Seq, PyEval, PyEvalRaw, PyExec, If, For };
	PymlWorkingItem::Type type;
	union {
		struct {
			std::string str;
		} strData;
		struct {
			std::vector<PymlWorkingItem*> items;
		} seqData;
		struct {
			std::string code;
		} pyCodeData;
		struct {
			std::string condition;
			PymlWorkingItem* itemIfTrue;
			PymlWorkingItem* itemIfFalse;
		} ifData;
		struct {
			std::string initCode;
			std::string conditionCode;
			std::string updateCode;
			PymlWorkingItem* loopItem;
		} forData;
	};
	
	PymlWorkingItem(Type type);
	~PymlWorkingItem();
	
	const PymlItem* getItem(PymlItemPool& pool) const;
};


class PymlFile {
	const PymlItem* rootItem;
	
	int state;
	std::string workingBackBuffer;
	char workingStr[65536];
	unsigned int workingIdx;
	
	std::stack<PymlWorkingItem> itemStack;
	PymlItemPool pool;
	
	const PymlItem* parseFromSource(const std::string& source);
	bool consumeOne(char chr);
public:
	PymlFile(const std::string& pymlSource):
		rootItem(parseFromSource(pymlSource)){
	}
	
	std::string runPyml() const;
};
