#include"pyml.h"
#include"pythonWorker.h"
#include"except.h"
#include"utils.h"
#include"fsm.h"

#include"dbg.h"


using namespace std;


string pythonPrepareStr(string pyCode);


string PymlItemSeq::runPyml() const {
	string result;
	for (const PymlItem* it : items){
		result += it->runPyml();
	}
	return result;
}


const PymlItem* PymlItemSeq::tryCollapse() const {
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
	pythonRun(code);
	return "";
}


std::string PymlItemIf::runPyml() const {
	if (pythonTest(conditionCode)){
		return itemIfTrue->runPyml();
	}
	else{
		return itemIfFalse->runPyml();
	}
}


std::string PymlItemFor::runPyml() const {
	pythonRun(initCode);
	string result;
	while(pythonTest(conditionCode)){
		result += loopItem->runPyml();
	}
	return result;
}


PymlWorkingItem::PymlWorkingItem(PymlWorkingItem::Type type)
	: data(NoneData()){
	DBG_FMT("In PymlWorkingItem constructor; type = %1%", (int)type);
	if (type == Type::None){
	}
	else if (type == Type::Str){
		data = StrData();
	}
	else if (type == Type::Seq){
		DBG("PreSeqData assign");
		data = SeqData();
		DBG("SeqData assigned");
	}
	else if (type == Type::PyEval || type == Type::PyEvalRaw || type == Type::PyExec){
		PyCodeData dataTmp;
		dataTmp.type = type;
		data = dataTmp;
	}
	else if (type == Type::If){
		data = IfData();
	}
	else if (type == Type::For){
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
			items.push_back(it->getItem(pool));
		}
		return pool.seqPool.construct(items);
	}
	const PymlItem* operator()(PymlWorkingItem::PyCodeData pyCodeData){
		if (pyCodeData.type == PymlWorkingItem::Type::PyExec){
			return pool.pyExecPool.construct(pyCodeData.code);
		}
		if (pyCodeData.type == PymlWorkingItem::Type::PyEval){
			return pool.pyEvalPool.construct(pyCodeData.code);
		}
		if (pyCodeData.type == PymlWorkingItem::Type::PyEvalRaw){
			return pool.pyEvalRawPool.construct(pyCodeData.code);
		}
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error parsing pyml file: Unrecognized type in PymlWorkingItem::PyCodeData."));
	}
	const PymlItem* operator()(PymlWorkingItem::IfData ifData){
		const PymlItem* itemIfTrue = ifData.itemIfTrue->getItem(pool);
		const PymlItem* itemIfFalse = ifData.itemIfFalse->getItem(pool);
		return pool.ifExecPool.construct(ifData.condition, itemIfTrue, itemIfFalse);
	}
	const PymlItem* operator()(PymlWorkingItem::ForData forData){
		const PymlItem* loopItem = forData.loopItem->getItem(pool);
		PymlItemFor* newItem = pool.forExecPool.malloc();
		PymlItemFor* item = new(newItem) PymlItemFor(forData.initCode, forData.conditionCode, forData.updateCode, loopItem);
		return item;
	}
};

const PymlItem* PymlWorkingItem::getItem(PymlItemPool& pool) const {
	GetItemVisitor visitor(pool);
	return boost::apply_visitor(visitor, data);
}






std::string PymlFile::runPyml() const {
	return rootItem->runPyml();
}


const PymlItem* PymlFile::parseFromSource(const std::string& source) {
	state = 0;
	memzero(workingStr);
	workingIdx = 0;
	workingBackBuffer.clear();
	
	while(itemStack.size() != 0){ //No clear() method...
		DBG_FMT("Popping; size is %1%", itemStack.size());
		itemStack.pop();
		
	}
	DBG_FMT("Pushing; size is %1%", itemStack.size());
	itemStack.push(PymlWorkingItem(PymlWorkingItem::Type::Seq));
	
	DBG("PreConsuming:");
	
	for (const char chr : source){
		DBG("consuming one");
		while(!consumeOne(chr)){
			DBG("consuming one, again");
		}
		DBG("consume one ok");
	}
	
	if (itemStack.size() != 1){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error parsing pyml file: Unexpected end. Tag not closed."));
	}
	
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error parsing pyml file: after consuming file, working item is not PymlWorkingItem::SeqData."));
	}
	
	if (workingBackBuffer.size() + workingIdx != 0){
		addPymlWorkingStr(workingBackBuffer + string(workingStr, workingIdx));
	}
	
	const PymlItem* result = itemStack.top().getItem(pool);
	return result;
}


bool PymlFile::consumeOne(char chr){
	DBG_FMT("In state %1%, consuming chr %2%", state, chr);
	string tmp;
	
	FsmStart(int, state, char, chr, workingStr, sizeof(workingStr), workingIdx, workingBackBuffer)
		StatesBegin(0)
			DBG("state 0");
			if (!stackTopIsType<PymlWorkingItem::SeqData>()){
				DBG("except!");
				BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq in state 0."));
			}
			DBG("no except!");
			//DBG_FMT("some pointers: ")
			SaveStart()
			DBG("started save");
			
			TransIf('<', 2, true)
			TransElse(1, true)
			
			DBG("trans okay");
		StatesNext(1)
			SaveThis()
			
			TransIf('<', 2, true)
			TransElse(1, true)
		StatesNext(2)
			SaveThis()
			
			TransIf('<', 2, true)
			TransElif('@', 3, true)
			TransElif('!', 4, true)
			//TransElif('?', 5, true) //Enable for if/for/etc support
			TransElse(1, true)
		StatesNext(3)
			SaveThis()
			
			TransIf(' ', 6, true)
			TransElif('!', 7, true)
			TransElse(1, true)
		StatesNext(4)
			SaveThis()
			
			TransIf(' ', 9, true)
			TransElse(1, true)
		StatesNext(5)
			BOOST_THROW_EXCEPTION(notImplementedError() << stringInfo("conditional support in pyml"));
		StatesNext(6) //Just consumed a space right before Python code for safe eval; python code follows
			SaveBackspace(3)
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
			SaveThis()
			
			TransIf('>', 22, true)
			TransElif('@', 21, true)
			TransElse(20, true)
		StatesNext(22)
			SaveBackspace(2)
			SaveStore(tmp)
			
			if (tmp.size() > 0){
				addPymlWorkingPyCode(PymlWorkingItem::Type::PyEval, tmp);
			}
			TransAlways(0, false)
		StatesNext(7)
			SaveThis()
			
			TransIf(' ', 10, true)
			TransElse(1, true)
		StatesNext(10)
			SaveBackspace(4)
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
			SaveThis()
			
			TransIf('!', 27, true)
			TransElif('@', 26, true)
			TransElse(25, true)
		StatesNext(27)
			SaveThis()
			
			TransIf('>', 28, true)
			TransElif('@', 26, true)
			TransElse(25, true)
		StatesNext(28)
			SaveBackspace(3)
			SaveStore(tmp)
			
			if (tmp.size() > 0){
				addPymlWorkingPyCode(PymlWorkingItem::Type::PyEvalRaw, tmp);
			}
			TransAlways(0, false)
		StatesNext(9)
			SaveBackspace(3)
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
			SaveThis()
			
			TransIf('>', 32, true)
			TransElif('!', 31, true)
			TransElse(30, true)
		StatesNext(32)
			SaveBackspace(2)
			SaveStore(tmp)
			
			if (tmp.size() > 0){
				addPymlWorkingPyCode(PymlWorkingItem::Type::PyExec, tmp);
			}
			TransAlways(0, false)
		StatesEnd()
	FsmEnd()
}


void PymlFile::addPymlWorkingStr(const std::string& str) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq in state 6."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(PymlWorkingItem::Type::Str);
	newItem->getData<PymlWorkingItem::StrData>()->str = str;
	
	data.items.push_back(newItem);
}


void PymlFile::addPymlWorkingPyCode(PymlWorkingItem::Type type, const std::string& code) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq in state 6."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(type);
	
	newItem->getData<PymlWorkingItem::PyCodeData>()->code = code;
	
	data.items.push_back(newItem);
}

//Dedent string
string pythonPrepareStr(string pyCode) {
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
	replacements[(int)'<'] = "&lt";
	replacements[(int)'>'] = "&gt";
	replacements[(int)'"'] = "&quot;";
	replacements[(int)'\''] = "&#39";

	unsigned int oldIdx = 0;
	for (unsigned int idx = 0; idx < htmlCode.length(); idx++) {
		if (replacements[(int)htmlCode[idx]] != NULL) {
			result += htmlCode.substr(oldIdx, idx - oldIdx) + replacements[(int)htmlCode[idx]];
			oldIdx = idx + 1;
		}
	}

	return result + htmlCode.substr(oldIdx, htmlCode.length() - oldIdx);
}
