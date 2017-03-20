#include "pymlItems.h"

#include"pythonWorker.h"
#include"except.h"
#include"utils.h"

#define DBG_DISABLE
#include "dbg.h"


using namespace std;

string PymlItemSeq::runPyml() const {
	string result;
	for (const PymlItem* it : items){
		result += it->runPyml();
	}
	return result;
}

bool PymlItemSeq::isDynamic() const {
	for (auto it: items){
		if (it->isDynamic()){
			return true;
		}
	}
	return false;
}

const IPymlItem* PymlItemSeq::getNext(const IPymlItem* last) const {
	if (last == NULL && items.size() != 0){
		return items[0];
	}

	for (int i = 0; i < (int)items.size(); i++){
		if (items[i] == last && i + 1 < (int)items.size()){
			return items[i + 1];
		}
	}

	return NULL;
}

const PymlItem* PymlItemSeq::tryCollapse() const {
	if (items.size() == 0){
		return NULL;
	}
	if (items.size() == 1){
		return items[0];
	}
	return (PymlItem*)this;
}

string htmlEscape(string htmlCode);

std::string PymlItemPyEval::runPyml() const {
	return htmlEscape(pythonEval(code));
}

std::string PymlItemPyEvalRaw::runPyml() const {
	return pythonEval(code);
}


std::string PymlItemPyExec::runPyml() const {
	DBG_FMT("running pyExec: %1%", code);
	pythonRun(code);
	return "";
}


std::string PymlItemIf::runPyml() const {
	if (pythonTest(conditionCode)){
		if (itemIfTrue == NULL){
			return "";
		}
		return itemIfTrue->runPyml();
	}
	else{
		if (itemIfFalse == NULL){
			return "";
		}
		return itemIfFalse->runPyml();
	}
}


const IPymlItem* PymlItemIf::getNext(const IPymlItem* last) const{
	if (last != NULL){
		return NULL;
	}

	if (pythonTest(conditionCode)){
		return itemIfTrue;
	}
	else{
		return itemIfFalse;
	}
}


std::string PymlItemFor::runPyml() const {
	pythonRun(initCode);
	string result;
	while(pythonTest(conditionCode)){
		result += loopItem->runPyml();
		pythonRun(updateCode);
	}
	return result;
}


const IPymlItem* PymlItemFor::getNext(const IPymlItem* last) const{
	if (last == NULL){
		pythonRun(initCode);
	}
	else{
		pythonRun(updateCode);
	}

	if (pythonTest(conditionCode)){
		return loopItem;
	}
	else{
		return NULL;
	}
}

const IPymlItem* PymlItemEmbed::getNext(const IPymlItem *last) const {
	if (last == NULL){
		return cache.get(filename)->getRootItem();
	}
	else {
		return NULL;
	}
}

PymlWorkingItem::PymlWorkingItem(PymlWorkingItem::Type type)
		: data(NoneData()){
	//DBG_FMT("In PymlWorkingItem constructor; type = %1%", (int)type);
	this->type = type;
	if (type == Type::None){
	}
	else if (type == Type::Str){
		data = StrData();
	}
	else if (type == Type::Seq){
		data = SeqData();
	}
	else if (type == Type::PyEval || type == Type::PyEvalRaw || type == Type::PyExec){
		PyCodeData dataTmp;
		dataTmp.type = type;
		data = dataTmp;
	}
	else if (type == Type::If){
		data = IfData();
	}
	else if (type == Type::For ){
		data = ForData();
	}
	else if (type == Type::Embed){
		data = EmbedData();
	}
	else{
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Server Error parsing pyml file: PymlWorkingItem type not recognized."));
	}
}


class GetItemVisitor : public boost::static_visitor<const PymlItem*>{
	PymlItemPool& pool;
public:
	GetItemVisitor(PymlItemPool& pool)
			: pool(pool){
	}

	const PymlItem* operator()(PymlWorkingItem::NoneData data){
		(void)data; //Silence the warning.
		return pool.itemPool.construct();
	}
	const PymlItem* operator()(PymlWorkingItem::StrData strData){
		return pool.strPool.construct(strData.str);
	}
	const PymlItem* operator()(PymlWorkingItem::SeqData seqData){
		vector<const PymlItem*> items;
		for (PymlWorkingItem* it : seqData.items){
			const PymlItem* item = it->getItem(pool);
			if (item != NULL){
				items.push_back(item);
			}
		}
		const PymlItemSeq* result = pool.seqPool.construct(items);
		return result->tryCollapse();
	}
	const PymlItem* operator()(PymlWorkingItem::PyCodeData pyCodeData){
		if (pyCodeData.type == PymlWorkingItem::Type::PyExec){
			return pool.pyExecPool.construct(pythonPrepareStr(pyCodeData.code));
		}
		if (pyCodeData.type == PymlWorkingItem::Type::PyEval){
			return pool.pyEvalPool.construct(pythonPrepareStr(pyCodeData.code));
		}
		if (pyCodeData.type == PymlWorkingItem::Type::PyEvalRaw){
			return pool.pyEvalRawPool.construct(pythonPrepareStr(pyCodeData.code));
		}
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error parsing pyml file: Unrecognized type in PymlWorkingItem::PyCodeData."));
	}
	const PymlItem* operator()(PymlWorkingItem::IfData ifData){
		const PymlItem* itemIfTrue = ifData.itemIfTrue->getItem(pool);
		const PymlItem* itemIfFalse = NULL;
		if (ifData.itemIfFalse != NULL){
			itemIfFalse = ifData.itemIfFalse->getItem(pool);
		}

		return pool.ifExecPool.construct(pythonPrepareStr(ifData.condition), itemIfTrue, itemIfFalse);
	}
	const PymlItem* operator()(PymlWorkingItem::ForData forData ){
		const PymlItem* loopItem = forData.loopItem->getItem(pool);
		PymlItemFor* newItem = pool.forExecPool.malloc();
		PymlItemFor* item = new(newItem) PymlItemFor(pythonPrepareStr(forData.initCode), forData.conditionCode, forData.updateCode, loopItem);
		return item;
	}

	const PymlItem* operator()(PymlWorkingItem::EmbedData embedData) {
		return pool.embedPool.construct(embedData.filename, *embedData.cache);
	}
};

const PymlItem* PymlWorkingItem::getItem(PymlItemPool& pool) const {
	GetItemVisitor visitor(pool);
	return boost::apply_visitor(visitor, data);
}