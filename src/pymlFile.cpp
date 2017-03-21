#include"dbg.h"

#include <boost/algorithm/string.hpp>
#include "pythonWorker.h"
#include "pymlFile.h"
#include"except.h"
#include"utils.h"
#include"fsm.h"
#include"path.h"

using namespace std;


PymlFile::PymlFile(PymlFile::SrcInfo srcInfo, PymlFile::CacheInfo cacheInfo)
		: cacheInfo(cacheInfo) {
	if (!srcInfo.isRaw){
		rootItem = parseFromSource(srcInfo.source);
	}
	else{
		rootItem = pool.strPool.construct(srcInfo.source);
	}
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
			TransElif('%', 90, true) //<%embed path %> / <%view path %vm path>
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
			//DBG("closing /f");
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
		StatesNext(90) //<%|embed ... | <%|view
			DBG("<%|");
			SaveThis()

			TransIf('e', 91, true)
			TransElse(1, true)
		StatesNext(91)
			SaveThis()

			TransIf('m', 92, true)
			TransElse(1, true)
		StatesNext(92)
			SaveThis()

			TransIf('b', 93, true)
			TransElse(1, true)
		StatesNext(93)
			SaveThis()

			TransIf('e', 94, true)
			TransElse(1, true)
		StatesNext(94)
			SaveThis()

			TransIf('d', 95, true)
			TransElse(1, true)
		StatesNext(95)
			SaveThis()

			TransIf(' ', 96, true)
			TransElse(1, true)
		StatesNext(96)
			DBG("<%embed|");
			SavepointRevert()
			SaveStore(tmp)
			SaveStart()

			if (tmp.size() > 0){
				addPymlWorkingStr(tmp);
			}

			TransIf('%', 98, true)
			TransElse(97, true)
		StatesNext(97)
			SaveThis()

			TransIf('%', 98, true)
			TransElse(97, true)
		StatesNext(98)
			SavepointStoreOff(-1)
			SaveThis()

			TransIf('%', 98, true)
			TransElif('>', 99, true)
			TransElse(97, true)
		StatesNext(99) //<%embed ... %>|
			SavepointRevert()

			SaveStore(tmp)
			boost::trim(tmp);
			DBG_FMT("added embed with code %1%", tmp);

			if (tmp.size() > 0){
				addPymlWorkingEmbed(tmp);
			}

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

void PymlFile::addPymlWorkingEmbed(const std::string &filename) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(PymlWorkingItem::Type::Embed);

	std::string newFilename = (cacheInfo.embedRoot / filename).string();
	DBG_FMT("embed filename is %1%, len %2%", newFilename, newFilename.length());
	DBG("calling checkExists()");
	pathCheckExists(newFilename);

	newItem->getData<PymlWorkingItem::EmbedData>()->filename = newFilename;
	newItem->getData<PymlWorkingItem::EmbedData>()->cache = &cacheInfo.cache;

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


string htmlEscape(string htmlCode) {
	string result;
	bool resultEmpty = true;

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
			if (resultEmpty){
				result.reserve(htmlCode.length() + htmlCode.length() / 10); //Approximately...
				resultEmpty = false;
			}

			result.append(htmlCode, oldIdx, idx - oldIdx);
			result.append(replacements[(int)htmlCode[idx]]);
			oldIdx = idx + 1;
		}
	}

	if (resultEmpty) {
		return htmlCode;
	}
	else {
		result.append(htmlCode.substr(oldIdx, htmlCode.length() - oldIdx));
		return result;
	}
}

