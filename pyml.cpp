#include"pyml.h"
#include"pythonWorker.h"
#include"except.h"
#include"utils.h"

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


const PymlItem * PymlWorkingItem::getItem(PymlItemPool& pool) const {
	if (type == None){
		return pool.itemPool.construct();
	}
	if (type == Type::Str){
		return pool.strPool.construct(strData.str);
	}
	if (type == Type::Seq){
		vector<const PymlItem*> items;
		for (PymlWorkingItem* it : seqData.items){
			items.push_back(it->getItem(pool));
		}
		return pool.seqPool.construct(items);
	}
	if (type == Type::PyEval){
		return pool.pyEvalPool.construct(pyCodeData.code);
	}
	if (type == Type::PyEvalRaw){
		return pool.pyEvalRawPool.construct(pyCodeData.code);
	}
	if (type == Type::PyExec){
		return pool.pyExecPool.construct(pyCodeData.code);
	}
	if (type == Type::If){
		const PymlItem* itemIfTrue = ifData.itemIfTrue->getItem(pool);
		const PymlItem* itemIfFalse = ifData.itemIfFalse->getItem(pool);
		return pool.ifExecPool.construct(ifData.condition, itemIfTrue, itemIfFalse);
	}
	if (type == Type::For){
		const PymlItem* loopItem = forData.loopItem->getItem(pool);
		PymlItemFor* newItem = pool.forExecPool.malloc();
		PymlItemFor* item = new(newItem) PymlItemFor(forData.initCode, forData.conditionCode, forData.updateCode, loopItem);
		return item;
	}
	BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml parser error: item type not found."));
}



std::string PymlFile::runPyml() const {
	return rootItem.runPyml();
}


PymlItem PymlFile::parseFromSource(const std::string& source) {
	state = 0;
	memzero(workingBackBuffer);
	workingIdx = 0;
	
	while(itemStack.size() != 0){ //No clear() method...
		itemStack.pop();
	}
	itemStack.push(PymlWorkingItem(PymlWorkingItem::Type::Seq));
	
	for (const char chr : source){
		consumeOne(chr);
	}
	
	if (itemStack.size() != 1){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error parsing pyml file: Unexpected end. Tag not closed."))
	}
}


bool PymlFile::consumeOne(char chr){
	
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
