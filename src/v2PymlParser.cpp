#include <boost/algorithm/string.hpp>
#include "except.h"
#include"utils.h"
#include"path.h"
#include "v2PymlParser.h"

#include "dbg.h"

using namespace std;


V2PymlParser::V2PymlParser(IPymlCache& cache, boost::filesystem::path embedRoot)
	: cache(cache), embedRoot(embedRoot){
	rootItem = NULL;
	krItIndex = 0;
}


void V2PymlParser::consume(std::string::iterator start, std::string::iterator end){
	BOOST_THROW_EXCEPTION(serverError() << stringInfo("v2 parser not implemented yet."));
}


IPymlItem* V2PymlParser::getParsed(){
	return rootItem;
}


void V2PymlParser::addPymlWorkingStr(const std::string& str) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(PymlWorkingItem::Type::Str);
	newItem->getData<PymlWorkingItem::StrData>()->str = str;

	data.items.push_back(newItem);
}


void V2PymlParser::addPymlWorkingPyCode(PymlWorkingItem::Type type, const std::string& code) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(type);

	newItem->getData<PymlWorkingItem::PyCodeData>()->code = code;

	data.items.push_back(newItem);
}

void V2PymlParser::addPymlWorkingEmbed(const std::string &filename) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(PymlWorkingItem::Type::Embed);

	std::string newFilename = (embedRoot / filename).string();
	DBG_FMT("embed filename is %1%, len %2%", newFilename, newFilename.length());
	DBG("calling checkExists()");
	pathCheckExists(newFilename);

	newItem->getData<PymlWorkingItem::EmbedData>()->filename = newFilename;
	newItem->getData<PymlWorkingItem::EmbedData>()->cache = &cache;

	data.items.push_back(newItem);
}

void V2PymlParser::pushPymlWorkingIf(const std::string& condition){
	itemStack.emplace(PymlWorkingItem::Type::If);
	itemStack.top().getData<PymlWorkingItem::IfData>()->condition = condition;
}

void V2PymlParser::pushPymlWorkingFor() {
	itemStack.emplace(PymlWorkingItem::Type::For);
}


void V2PymlParser::pushPymlWorkingSeq() {
	itemStack.emplace(PymlWorkingItem::Type::Seq);
}

bool V2PymlParser::addSeqToPymlWorkingIf(bool isElse){
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		return false;
	}
	PymlWorkingItem& itemSrc = itemStack.top();
	PymlWorkingItem* item = workingItemPool.construct(PymlWorkingItem::Type::Seq);
	*item = itemSrc;
	itemStack.pop();

	if (itemStack.empty()){
		return false;
	}

	PymlWorkingItem::IfData* data = itemStack.top().getData<PymlWorkingItem::IfData>();
	if (data == NULL){
		DBG_FMT("Bad item data; expected IfData, got %1%", itemStack.top().type);
		return false;
	}

	if (isElse){
		if (data->itemIfFalse != NULL){
			return false;
		}
		else{
			data->itemIfFalse = item;
			return true;
		}
	}
	else{
		if (data->itemIfTrue != NULL){
			return false;
		}
		else{
			data->itemIfTrue = item;
			return true;
		}
	}
}

bool V2PymlParser::addSeqToPymlWorkingFor() {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		return false;
	}
	PymlWorkingItem& itemSrc = itemStack.top();
	PymlWorkingItem* item = workingItemPool.construct(PymlWorkingItem::Type::Seq);
	*item = itemSrc;
	itemStack.pop();

	if (itemStack.empty()){
		return false;
	}

	PymlWorkingItem::ForData* data = itemStack.top().getData<PymlWorkingItem::ForData>();
	if (data == NULL){
		DBG_FMT("Bad item data; expected ForData, got %1%", itemStack.top().type);
		return false;
	}

	data->loopItem = item;
	return true;
}


void V2PymlParser::addCodeToPymlWorkingFor(int where, const std::string& code) {
	if (!stackTopIsType<PymlWorkingItem::ForData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Parser error: tried to add code to <?for ?> without a for being on top"));
	}

	PymlWorkingItem::ForData& data = getStackTop<PymlWorkingItem::ForData>();

	if (where == 0){
		data.initCode = code;
	}
	else if (where == 1){
		data.conditionCode = code;
	}
	else if (where == 2){
		data.updateCode = code;
	}
}

void V2PymlParser::addPymlStackTop() {
	PymlWorkingItem& itemSrc = itemStack.top();
	PymlWorkingItem* newItem = workingItemPool.construct(itemSrc.type);
	*newItem = itemSrc;
	itemStack.pop();

	if (itemStack.empty()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Tried reducing a stack with just one item."));
	}

	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Pyml FSM error: stack top not Seq, but %1%", itemStack.top().type));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();

	data.items.push_back(newItem);
}

void V2PymlParser::pushPymlWorkingForIn(std::string entry, std::string collection){
	//DBG_FMT("for in with entries %1% and %2%", entry, collection);

	string krIterator = (boost::format("_krIt%d") % (krItIndex++)).str();
	boost::trim(entry);
	boost::trim(collection);

	//1: krIterator; 2: collection;;; 3: entry
	string initCode = (boost::format("%1% = krait.IteratorWrapper(%2%)\nif not %1%.over: %3% = %1%.value")
	                   % krIterator % collection % entry).str();
	string condCode = (boost::format("not %1%.over") % krIterator).str();
	string updateCode = (boost::format("%1%.next()\nif not %1%.over: %3% = %1%.value")
	                     % krIterator % collection % entry).str();

	addCodeToPymlWorkingFor(0, initCode);
	addCodeToPymlWorkingFor(1, condCode);
	addCodeToPymlWorkingFor(2, updateCode);

	//DBG("done!");
}