#define DBG_DISABLE
#include"dbg.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include"pyml.h"
#include"pythonWorker.h"
#include"except.h"
#include"utils.h"
#include"fsm.h"


using namespace std;


string pythonPrepareStr(string pyCode);


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
	string evalResult = pythonEval(code);
	return htmlEscape(evalResult);
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


std::string PymlItemFor::runPyml() const {
	pythonRun(initCode); //todo:remove this or back to for
	string result;
	while(pythonTest(conditionCode)){
		result += loopItem->runPyml();
		pythonRun(updateCode);
	}
	return result;
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
};

const PymlItem* PymlWorkingItem::getItem(PymlItemPool& pool) const {
	GetItemVisitor visitor(pool);
	return boost::apply_visitor(visitor, data);
}


std::string PymlFile::runPyml() const {
	if (rootItem == NULL){
		return "";
	}
	return rootItem->runPyml();
}

bool PymlFile::isDynamic() const {
	if (rootItem == NULL){
		return false;
	}
	return rootItem->isDynamic();
}


const PymlItem* PymlFile::parseFromSource(const std::string& source) {
	state = 0;
	memzero(workingStr);
	workingIdx = 0;
	absIdx = 0;
	saveIdx = 0;
	workingBackBuffer.clear();
	
	krItIndex = 0;
	
	while(itemStack.size() != 0){ //No clear() method...
		//DBG_FMT("Popping; size is %1%", itemStack.size());
		itemStack.pop();
	}
	//DBG_FMT("Pushing; size is %1%", itemStack.size());
	itemStack.emplace(PymlWorkingItem::Type::Seq);
	
	//DBG("PreConsuming:");
	
	for (const char chr : source){
		//DBG("consuming one");
		while(!consumeOne(chr)){
			//DBG("consuming one, again");
		}
		//DBG("consume one ok");
	}
	
	consumeOne('\0'); //Required to close all pending states.
	
	if (itemStack.size() != 1){
		DBG_FMT("Error unexpected end! state is %1%", state);
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error parsing pyml file: Unexpected end. Tag not closed."));
	}
	
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		DBG_FMT("Error top not Seq! State is %1%", state);
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error parsing pyml file: after consuming file, working item is not PymlWorkingItem::SeqData."));
	}
	
	if (workingBackBuffer.size() + workingIdx != 0){
		string tmp = workingBackBuffer + string(workingStr, workingIdx);
		if (tmp[tmp.length() - 1] == '\0'){
			tmp.erase(tmp.length() - 1);
		}
		addPymlWorkingStr(tmp);
	}
	
	const PymlItem* result = itemStack.top().getItem(pool);
	return result;
}


PymlFile::PymlFile(const string& source, bool isRaw) {
	if (!isRaw){
		rootItem = parseFromSource(source);
	}
	else{
		rootItem = pool.strPool.construct(source);
	}
}


bool PymlFile::consumeOne(char chr){
	//DBG_FMT("In state %1%, consuming chr %2%", state, chr);
	string tmp;
	
	FsmStart(int, state, char, chr, workingStr, sizeof(workingStr), workingIdx, workingBackBuffer, &absIdx, &saveIdx)
		StatesBegin(0)
			//DBG("state 0");
			if (!stackTopIsType<PymlWorkingItem::SeqData>()){
				//DBG("except!");
				BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq in state 0."));
			}
			//DBG("no except!");
			//DBG_FMT("some pointers: ")
			SaveStart()
			//DBG("started save");
			
			TransIf('<', 2, true)
			TransElse(1, true)
			
			//DBG("trans okay");
		StatesNext(1)
			SaveThis()
			
			TransIf('<', 2, true)
			TransElse(1, true)
		StatesNext(2)
			SavepointStoreOff(-1)
			SaveThis()
			
			TransIf('<', 2, true)
			TransElif('/', 11, true)
			TransElif('@', 3, true)
			TransElif('!', 4, true)
			TransElif('?', 5, true) //Enable for if/while/etc support
			TransElse(1, true)
		StatesNext(3)
			SaveThis()
			
			TransIfWs(6, true)
			TransElif('!', 7, true)
			TransElse(1, true)
		StatesNext(4)
			SaveThis()
			
			TransIfWs(9, true)
			TransElse(1, true)
		StatesNext(5)
			//BOOST_THROW_EXCEPTION(notImplementedError() << stringInfo("conditional support in pyml"));
			SaveThis()
			
			TransIf('i', 50, true)
			TransElif('f', 60, true)
			TransElse(1, true)
		StatesNext(6) //Just consumed a space right before Python code for safe eval; python code follows
			SavepointRevert()
			SaveStore(tmp)
			SaveStart()
			
			if (tmp.size() > 0){
				addPymlWorkingStr(tmp);
			}
			TransAlways(20, true)
		StatesNext(20)
			SaveThis()
			
			TransIf('@', 21, true)
			TransElse(20, true)
		StatesNext(21)
			SavepointStoreOff(-1)
			SaveThis()
			
			TransIf('>', 22, true)
			TransElif('@', 21, true)
			TransElse(20, true)
		StatesNext(22)
			SavepointRevert()
			SaveStore(tmp)
			SaveDiscard()
			
			if (tmp.size() > 0){
				DBG_FMT("added eval code %1%", pythonPrepareStr(tmp));
				addPymlWorkingPyCode(PymlWorkingItem::Type::PyEval, pythonPrepareStr(tmp));
			}
			TransAlways(0, false)
 		StatesNext(7) //eval raw
			SaveThis()
			
			TransIfWs(10, true)
			TransElse(1, true)
		StatesNext(10)
			SavepointRevert()
			SaveStore(tmp)
			SaveStart()
			
			if (tmp.size() > 0){
				addPymlWorkingStr(tmp);
			}
			TransAlways(25, true)
		StatesNext(25)
			SaveThis()
			
			TransIf('@', 26, true)
			TransElse(25, true)
		StatesNext(26)
			SavepointStoreOff(-1)
			SaveThis()
			
			TransIf('>', 27, true)
			TransElif('@', 26, true)
			TransElse(25, true)
		StatesNext(27)
			SavepointRevert()
			SaveStore(tmp)
			SaveDiscard()
			
			if (tmp.size() > 0){
				DBG_FMT("added raw eval code %1%", pythonPrepareStr(tmp));
				addPymlWorkingPyCode(PymlWorkingItem::Type::PyEvalRaw, pythonPrepareStr(tmp));
			}
			TransAlways(0, false)
		StatesNext(9)
			SavepointRevert()
			SaveStore(tmp)
			SaveStart()
			
			if (tmp.size() > 0){
				addPymlWorkingStr(tmp);
			}
			TransAlways(30, true)
		StatesNext(30)
			SaveThis()
			
			TransIf('!', 31, true)
			TransElse(30, true)
		StatesNext(31)
			SavepointStoreOff(-1)
			SaveThis()
			
			TransIf('>', 32, true)
			TransElif('!', 31, true)
			TransElse(30, true)
		StatesNext(32)
			SavepointRevert()
			SaveStore(tmp)
			SaveDiscard()
			
			if (tmp.size() > 0){
				DBG_FMT("added exec code %1%", pythonPrepareStr(tmp));
				addPymlWorkingPyCode(PymlWorkingItem::Type::PyExec, pythonPrepareStr(tmp));
			}
			TransAlways(0, false)
		StatesNext(50)
			SaveThis()
		
			TransIf('f', 51, true)
			TransElse(1, true)
		StatesNext(51)
			SaveThis()
			
			TransIfWs(52, true)
			TransElse(1, true)
		StatesNext(52) //just consumed space before if condition
			SavepointRevert()
			SaveStore(tmp)
			SaveStart()
			
			if (tmp.size() > 0){
				addPymlWorkingStr(tmp);
			}
			
			TransAlways(53, true)
		StatesNext(53)
			SaveThis()
			
			TransIf('?', 54, true)
			TransElse(53, true)
		StatesNext(54)
			SavepointStoreOff(-1)
			SaveThis()
			
			TransIf('?', 54, true)
			TransElif('>', 55, true)
			TransElse(53, true)
		StatesNext(55)
			SavepointRevert()
			SaveStore(tmp);
			SaveDiscard()
			
			if (tmp.size() == 0){
				BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml parsing error: if condition missing!"));
			}
			DBG_FMT("Added if with condition %1%", tmp);
			pushPymlWorkingIf(pythonPrepareStr(tmp));
			pushPymlWorkingSeq();
			
			TransAlways(0, false)
		StatesNext(11) //"</"
			SaveThis()
			
			TransIf('i', 56, true)
			TransElif('f', 73, true)
			TransElse(1, true)
		StatesNext(56) //"</?"
			SaveThis()
			
			TransIf('f', 57, true)
			TransElse(1, true)
		StatesNext(57)
			SaveThis()
			
			TransIf('>', 58, true)
			TransElifWs(57, true)
			TransElse(1, true)
		StatesNext(58)
			SavepointRevert()
			SaveStore(tmp)
			SaveDiscard()
			
			if (tmp.size() > 0){
				addPymlWorkingStr(tmp);
			}
			
			if (!addSeqToPymlWorkingIf(false)){
				BOOST_THROW_EXCEPTION(serverError() << stringInfo("Unexpected </if>"));
			}
			addPymlStackTop();
			
			TransAlways(0, false)
		StatesNext(60)
			SaveThis()
		
			TransIf('o', 61, true)
			TransElse(1, true)
		StatesNext(61)
			SaveThis()
			
			TransIf('r', 62, true)
			TransElse(1, true)
		StatesNext(62)
			SaveThis()
			
			TransIfWs(63, true)
			TransElse(1, true)
		StatesNext(63) //first character of for init
			SavepointRevert()
			SaveStore(tmp)
			SaveStart()
			
			if (tmp.size() > 0){
				addPymlWorkingStr(tmp);
			}
			pushPymlWorkingFor();
			
			TransIf('?', 65, true)
			TransElse(64, true)
		StatesNext(64) //for init
			SaveThis()
			
			TransIf('?', 65, true)
			TransElse(64, true)
		StatesNext(65)
			SavepointStoreOff(-1)
			SaveThis()
			
			TransIf(';', 66, true)
			TransElif('i', 77, true)
			TransElif('?', 65, true)
			TransElse(64, true)
		StatesNext(66) //finished for init
			SavepointRevert()
			SaveStore(tmp)
			SaveStart()
			
			if (tmp.size() > 0){
				addCodeToPymlWorkingFor(0, pythonPrepareStr(tmp));
			}
			
			TransIf('?', 68, true)
			TransElse(67, true)
		StatesNext(67) //for cond
			SaveThis()
			
			TransIf('?', 68, true)
			TransElse(67, true)
		StatesNext(68)
			SavepointStoreOff(-1)
			SaveThis()
			
			TransIf(';', 69, true) //TODO: '>?'
			TransElif('?', 68, true)
			TransElse(67, true)
		StatesNext(69) //finished for cond
			SavepointRevert()
			SaveStore(tmp)
			SaveStart()
			
			if (tmp.size() > 0){
				addCodeToPymlWorkingFor(1, pythonPrepareStr(tmp));
			}
			
			TransIf('?', 71, true)
			TransElse(70, true)
		StatesNext(70) //for update
			SaveThis()
			
			TransIf('?', 71, true)
			TransElse(70, true)
		StatesNext(71)
			SavepointStoreOff(-1)
			SaveThis()
			
			TransIf('>', 72, true)
			TransElif('?', 71, true)
			TransElse(70, true)
		StatesNext(72) //finished for update
			SavepointRevert()
			SaveStore(tmp)
			SaveDiscard()
			
			if (tmp.size() > 0){
				addCodeToPymlWorkingFor(2, pythonPrepareStr(tmp));
			}
			pushPymlWorkingSeq();
			
			TransAlways(0, false)
		StatesNext(73) //"</f"
			DBG("closing /f");
			SaveThis()
			
			TransIf('o', 74, true)
			TransElse(1, true)
		StatesNext(74) //"</fo"
			SaveThis()
			
			TransIf('r', 75, true)
			TransElse(1, true)
		StatesNext(75)
			SaveThis()
			
			TransIfWs(75, true)
			TransElif('>', 76, true)
			TransElse(1, true)
		StatesNext(76)
			SavepointRevert()
			SaveStore(tmp)
			SaveDiscard()
			//DBG("closed for okay");
			
			if (tmp.size() > 0){
				addPymlWorkingStr(tmp);
			}
			
			if (!addSeqToPymlWorkingFor()){
				BOOST_THROW_EXCEPTION(serverError() << stringInfo("Unexpected </for>"));
			}
			addPymlStackTop();
			
			TransAlways(0, false)
		StatesNext(77) //for ... ?i
			SaveThis()
			
			TransIf('n', 78, true)
			TransElse(64, true)
		StatesNext(78) //for ... ?in
			SavepointRevert()
			SaveStore(tmpStr) //member string
			SaveStart()
		
			TransIf('?', 80, true)
			TransElse(79, true)
		StatesNext(79) //for entry ?in iterator
			SaveThis()
			
			TransIf('?', 80, true)
			TransElse(79, true)
		StatesNext(80) //for entry ?in iterator ?
			SavepointStoreOff(-1)
			SaveThis()
			
			TransIf('>', 81, true)
			TransElif('?', 80, true)
			TransElse(79, true)
		StatesNext(81) //for entry ?in iterator ?>
			SavepointRevert()
			SaveStore(tmp)
			SaveDiscard()
			
			pushPymlWorkingForIn(pythonPrepareStr(tmpStr), pythonPrepareStr(tmp));
			pushPymlWorkingSeq();
			
			TransAlways(0, false)
		StatesEnd()
	FsmEnd()
}


void PymlFile::addPymlWorkingStr(const std::string& str) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(PymlWorkingItem::Type::Str);
	newItem->getData<PymlWorkingItem::StrData>()->str = str;
	
	data.items.push_back(newItem);
}


void PymlFile::addPymlWorkingPyCode(PymlWorkingItem::Type type, const std::string& code) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(type);
	
	newItem->getData<PymlWorkingItem::PyCodeData>()->code = code;
	
	data.items.push_back(newItem);
}

void PymlFile::pushPymlWorkingIf(const std::string& condition){
	itemStack.emplace(PymlWorkingItem::Type::If);
	itemStack.top().getData<PymlWorkingItem::IfData>()->condition = condition;
}

void PymlFile::pushPymlWorkingFor() {
	itemStack.emplace(PymlWorkingItem::Type::For);
}

void PymlFile::pushPymlWorkingSeq() {
	itemStack.emplace(PymlWorkingItem::Type::Seq);
}


bool PymlFile::addSeqToPymlWorkingIf(bool isElse){
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

bool PymlFile::addSeqToPymlWorkingFor() {
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

void PymlFile::addCodeToPymlWorkingFor(int where, const std::string& code) {
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


void PymlFile::addPymlStackTop() {
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

void PymlFile::pushPymlWorkingForIn(std::string entry, std::string collection){
	DBG_FMT("for in with entries %1% and %2%", entry, collection);
	
	string krIterator = (boost::format("_krIt%d") % (krItIndex++)).str();
	boost::trim(entry);
	boost::trim(collection);
	
	//1: krIterator; 2: collection;;; 3: entry
	string initCode = (boost::format("%1% = IteratorWrapper(%2%)\nif not %1%.over: %3% = %1%.value") % krIterator % collection % entry).str();
	string condCode = (boost::format("not %1%.over") % krIterator).str();
	string updateCode = (boost::format("%1%.next()\nif not %1%.over: %3% = %1%.value") % krIterator % collection % entry).str();
	
	addCodeToPymlWorkingFor(0, initCode);
	addCodeToPymlWorkingFor(1, condCode);
	addCodeToPymlWorkingFor(2, updateCode);

	DBG("done!");
}



//Dedent string
string pythonPrepareStr(string pyCode) {
	if (pyCode.length() == 0 || !isspace(pyCode[0])){
		return pyCode;
	}
		
	string result = pyCode;
	//First remove first line if whitespace.

	unsigned int idx;
	bool foundNewline = false;
	for (idx = 0; idx < result.length(); idx++) {
		if (foundNewline && result[idx] != '\r' && result[idx] != '\n') {
			break;
		}
		if (!isspace(result[idx])) {
			idx = 0;
			break;
		}
		if (result[idx] == '\n') {
			foundNewline = true;
		}
	}
	unsigned int codeStartIdx = idx;

	if (codeStartIdx == result.length()) {
		return "";
	}

	char indentChr = result[codeStartIdx];
	for (idx = codeStartIdx; idx < result.length(); idx++) {
		if (result[idx] != indentChr) {
			break;
		}
	}
	int indentRemove = idx - codeStartIdx;
	if (indentRemove == 0) {
		return pyCode;
	}
	//DBG_FMT("Indent is %1%", indentRemove);

	for (idx = codeStartIdx; idx < result.length(); idx++) {
		int idx2;
		for (idx2 = 0; idx2 < indentRemove; idx2++) {
			if (result[idx + idx2] == '\r') {
				continue;
			}
			if (result[idx + idx2] == '\n') {
				//DBG("Premature break in dedent");
				break;
			}
			if (result[idx + idx2] != indentChr) {
				BOOST_THROW_EXCEPTION(serverError() <<
				                      stringInfoFromFormat("Unexpected character '%c' (\\x%02x) at pos %d in indent before Python code; expected '%c' (\\x%02x)",
				                              result[idx + idx2], (int) result[idx + idx2], idx + idx2, indentChr, (int)indentChr));
			}
		}
		result.erase(idx, idx2);
		for (; idx < result.length(); idx++) {
			if (result[idx] == '\n') {
				break;
			}
		}
	}

	return result;
}


string htmlEscape(string htmlCode) {
	string result;

	const char* replacements[256];
	memzero(replacements);
	replacements[(int)'&'] = "&amp;";
	replacements[(int)'<'] = "&lt;";
	replacements[(int)'>'] = "&gt;";
	replacements[(int)'"'] = "&quot;";
	replacements[(int)'\''] = "&#39;";

	unsigned int oldIdx = 0;
	for (unsigned int idx = 0; idx < htmlCode.length(); idx++) {
		if (replacements[(int)htmlCode[idx]] != NULL) {
			result += htmlCode.substr(oldIdx, idx - oldIdx) + replacements[(int)htmlCode[idx]];
			oldIdx = idx + 1;
		}
	}

	return result + htmlCode.substr(oldIdx, htmlCode.length() - oldIdx);
}
