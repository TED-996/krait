#include <boost/algorithm/string.hpp>
#include "except.h"
#include"utils.h"
#include"path.h"
#include "v2PymlParser.h"

#define DBG_DISABLE
#include "dbg.h"

V2PymlParserFsm V2PymlParser::parserFsm;


V2PymlParser::V2PymlParser(IPymlCache& cache)
	: cache(cache) {
	rootItem = nullptr;
	krItIndex = 0;
}


void V2PymlParser::consume(std::string::iterator start, std::string::iterator end) {
	while (itemStack.size() != 0) {
		itemStack.pop();
	}
	itemStack.emplace(PymlWorkingItem::Type::Seq);

	parserFsm.reset();
	parserFsm.setParser(this);
	while (start != end) {
		parserFsm.consumeOne(*start);
		++start;
	}

	parserFsm.doFinalPass();

	if (itemStack.size() != 1) {
		DBG_FMT("Error unexpected end! state is %1%", parserFsm.getState());
		BOOST_THROW_EXCEPTION(pymlError() << stringInfo("Error parsing pyml file: Unexpected end, tag not closed."));
	}

	if (!stackTopIsType<PymlWorkingItem::SeqData>()) {
		DBG_FMT("Error top not Seq! State is %1%", parserFsm.getState());
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Error parsing pyml file: after consuming file, working item is not PymlWorkingItem::SeqData."));
	}

	rootItem = itemStack.top().getItem();
}


std::unique_ptr<const IPymlItem> V2PymlParser::getParsed() {
	if (rootItem == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml parser: root null (double consumption)"));
	}

	return std::move(rootItem);
}


void V2PymlParser::addPymlWorkingStr(const std::string& str) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(PymlWorkingItem::Type::Str);
	newItem->getData<PymlWorkingItem::StrData>()->str = str;

	data.items.push_back(newItem);
}

bool isWhitespace(const std::string& str) {
	for (auto ch : str) {
		if (!std::isspace(ch)) {
			return false;
		}
	}
	return true;
}


void V2PymlParser::addPymlWorkingPyCode(PymlWorkingItem::Type type, const std::string& code) {
	if (code.length() == 0 || isWhitespace(code)) {
		return;
	}

	if (!stackTopIsType<PymlWorkingItem::SeqData>()) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(type);

	newItem->getData<PymlWorkingItem::PyCodeData>()->code = code;

	data.items.push_back(newItem);
}

void V2PymlParser::addPymlWorkingEmbed(const std::string& filename) {
	if (filename.length() == 0 || isWhitespace(filename)) {
		BOOST_THROW_EXCEPTION(pymlError() << stringInfo("Import filename code empty."));
	}

	//DBG_FMT("added embed %1%", filename);
	if (!stackTopIsType<PymlWorkingItem::SeqData>()) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();
	PymlWorkingItem* newItem = workingItemPool.construct(PymlWorkingItem::Type::Embed);

	std::string newFilename = formatString("krait.get_full_path(%1%)", filename);

	newItem->getData<PymlWorkingItem::EmbedData>()->filename = newFilename;
	newItem->getData<PymlWorkingItem::EmbedData>()->cache = &cache;

	data.items.push_back(newItem);
}

void V2PymlParser::addPymlWorkingCtrl(const std::string& ctrlCode) {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml FSM error: stack top not Seq."));
	}

	if (ctrlCode.length() == 0 || isWhitespace(ctrlCode)) {
		BOOST_THROW_EXCEPTION(pymlError() << stringInfo("import-ctrl controller code empty."));
	}

	pushPymlWorkingSeq();

	addPymlWorkingPyCode(PymlWorkingItem::Type::PyExec, formatString("ctrl = krait.mvc.push_ctrl((%1%))", ctrlCode));
	addPymlWorkingEmbed("ctrl.get_view()");
	addPymlWorkingPyCode(PymlWorkingItem::Type::PyExec, "ctrl = krait.mvc.pop_ctrl()");

	addPymlStackTop();
}

void V2PymlParser::pushPymlWorkingIf(const std::string& condition) {
	if (condition.length() == 0 || isWhitespace(condition)) {
		BOOST_THROW_EXCEPTION(pymlError() << stringInfo("If condition code empty."));
	}

	itemStack.emplace(PymlWorkingItem::Type::If);
	itemStack.top().getData<PymlWorkingItem::IfData>()->condition = condition; //TODO: prepare + trim !!IMPORTANT
}


void V2PymlParser::pushPymlWorkingFor() {
	itemStack.emplace(PymlWorkingItem::Type::For);
}

void V2PymlParser::pushPymlWorkingSeq() {
	itemStack.emplace(PymlWorkingItem::Type::Seq);
}

bool V2PymlParser::addSeqToPymlWorkingIf() {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()) {
		return false;
	}
	PymlWorkingItem& itemSrc = itemStack.top();
	PymlWorkingItem* item = workingItemPool.construct(PymlWorkingItem::Type::Seq);
	*item = itemSrc;
	itemStack.pop();

	if (itemStack.empty()) {
		return false;
	}

	PymlWorkingItem::IfData* data = itemStack.top().getData<PymlWorkingItem::IfData>();
	if (data == NULL) {
		DBG_FMT("Bad item data; expected IfData, got %1%", itemStack.top().type);
		return false;
	}

	if (data->itemIfTrue == nullptr) {
		data->itemIfTrue = item;
		return true;
	}
	else if (data->itemIfFalse == nullptr) {
		data->itemIfFalse = item;
		return true;
	}
	else {
		return false;
	}
}


bool V2PymlParser::addSeqToPymlWorkingFor() {
	if (!stackTopIsType<PymlWorkingItem::SeqData>()) {
		return false;
	}
	PymlWorkingItem& itemSrc = itemStack.top();
	PymlWorkingItem* item = workingItemPool.construct(PymlWorkingItem::Type::Seq);
	*item = itemSrc;
	itemStack.pop();

	if (itemStack.empty()) {
		return false;
	}

	PymlWorkingItem::ForData* data = itemStack.top().getData<PymlWorkingItem::ForData>();
	if (data == NULL) {
		DBG_FMT("Bad item data; expected ForData, got %1%", itemStack.top().type);
		return false;
	}

	data->loopItem = item;
	return true;
}

void V2PymlParser::addCodeToPymlWorkingFor(int where, const std::string& code) {
	if (!stackTopIsType<PymlWorkingItem::ForData>()) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Parser error: tried to add code to <?for ?> without a for being on top"));
	}
	if (code.length() == 0 || isWhitespace(code)) {
		return;
	}

	PymlWorkingItem::ForData& data = getStackTop<PymlWorkingItem::ForData>();

	if (where == 0) {
		data.initCode = code;
	}
	else if (where == 1) {
		data.conditionCode = code;
	}
	else if (where == 2) {
		data.updateCode = code;
	}
}

void V2PymlParser::addPymlStackTop() {
	PymlWorkingItem& itemSrc = itemStack.top();
	PymlWorkingItem* newItem = workingItemPool.construct(itemSrc.type);
	*newItem = itemSrc;
	itemStack.pop();

	if (itemStack.empty()) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Tried reducing a stack with just one item."));
	}

	if (!stackTopIsType<PymlWorkingItem::SeqData>()) {
		BOOST_THROW_EXCEPTION(
			serverError() <<
			stringInfoFromFormat("Pyml FSM error: stack top not Seq, but %1%", itemStack.top().type));
	}
	PymlWorkingItem::SeqData& data = getStackTop<PymlWorkingItem::SeqData>();

	data.items.push_back(newItem);
}

void V2PymlParser::pushPymlWorkingForIn(const std::string& entry, const std::string& collection) {
	if (entry.length() == 0 || isWhitespace(entry) || collection.length() == 0 || isWhitespace(collection)) {
		BOOST_THROW_EXCEPTION(pymlError() << stringInfo("For collection/entry code empty."));
	}

	std::string krIterator = (boost::format("_krIt%d") % (krItIndex++)).str();
	std::string entryTrimmed = boost::trim_copy(entry);
	std::string collectionTrimmed = boost::trim_copy(collection);

	//1: krIterator; 2: collection;;; 3: entry
	std::string initCode = (boost::format("%1% = IteratorWrapper(%2%)\nif not %1%.over: %3% = %1%.value")
		% krIterator % collectionTrimmed % entryTrimmed).str();
	std::string condCode = (boost::format("not %1%.over") % krIterator).str();
	std::string updateCode = (boost::format("%1%.next()\nif not %1%.over: %2% = %1%.value")
		% krIterator % entryTrimmed).str();
	//TODO: delete the IteratorWrapper! over time these add up (esp. with runtime reuse)!

	pushPymlWorkingFor();
	addCodeToPymlWorkingFor(0, initCode);
	addCodeToPymlWorkingFor(1, condCode);
	addCodeToPymlWorkingFor(2, updateCode);
}


V2PymlParserFsm::V2PymlParserFsm()
	: FsmV2(30, 60) { //TODO: tune
	parser = nullptr;
	init();
}

void V2PymlParserFsm::init() {
	typedef SimpleFsmTransition Simple;
	typedef ActionFsmTransition Action;
	typedef SavepointSetFsmTransition SavepointSet;
	typedef SavepointRevertFsmTransition SavepointRevert;
	typedef PushFsmTransition Push;
	typedef DiscardFsmTransition Discard;
	typedef AlwaysFsmTransition Always;
	typedef AndConditionFsmTransition Condition;
	typedef SkipFsmTransition Skip;
	typedef WhitespaceFsmTransition Whitespace;
	typedef OrFinalFsmTransition OrFinal;

	typedef PymlAddStrTransition PymlAddStr;

	const size_t start = 0;
	const size_t atRoot = 1;
	const size_t atRoot2 = 2;
	const size_t execRoot = 3;
	const size_t evalRoot = 4;
	const size_t evalRawRoot = 5;

	const size_t atI = 6;
	const size_t atIf = 7;
	const size_t ifRoot = 8;
	const size_t elseRoot = 9;

	const size_t atFor = 10;
	const size_t forEntry = 11;
	const size_t forPreIn = 12;
	const size_t forPostIn = 13;
	const size_t forCollection = 14;

	const size_t atSlash = 15;
	const size_t endIf = 16;
	const size_t endFor = 17;

	const size_t atImport = 18;
	const size_t importRoot = 19;

	const size_t atImportCtrl = 20;
	const size_t importCtrlRoot = 21;

	std::string bracketDepthKey = "bracketDepth";

	add(start, new SavepointSet(new Simple('@', atRoot)));
	add(start, new Always(start));

	add(atRoot, new SavepointRevert(new Simple('@', start)));
	add(atRoot, new SavepointSet(new PymlAddStr(&parser, new SavepointRevert(new Always(atRoot2, false)))));

	// @{...}
	add(atRoot2, new Skip(new Action(new Simple('{', execRoot), [=](FsmV2& fsm) { fsm.setProp(bracketDepthKey, 0); })));
	//Finish @{...}
	add(execRoot, new PymlAddPyCodeTransition(&parser,
	                                          new Skip(new Condition(new Simple('}', start),
	                                                                 [=](char chr, FsmV2& fsm) {
	                                                                return fsm.getProp(bracketDepthKey) == 0;
                                                                })),
	                                          PymlWorkingItem::Type::PyExec));
	//Increase depth
	add(execRoot, new Action(new Simple('{', execRoot), [=](FsmV2& fsm) {
	                        fsm.setProp(bracketDepthKey, fsm.getProp(bracketDepthKey) + 1);
                        }));
	//Decrease depth
	add(execRoot, new Action(new Condition(new Simple('}', execRoot),
	                                       [=](char chr, FsmV2& fsm) { return fsm.getProp(bracketDepthKey) != 0; }),
	                         [=](FsmV2& fsm) { fsm.setProp(bracketDepthKey, fsm.getProp(bracketDepthKey) - 1); }));
	//escape strings
	addStringLiteralParser(execRoot, execRoot, '\"', '\\');
	addStringLiteralParser(execRoot, execRoot, '\'', '\\');
	//continue PyExec
	add(execRoot, new Always(execRoot));

	// @if
	// Start with an 'i'
	add(atRoot2, new Simple('i', atI));
	// Continue with an 'f'
	add(atI, new Simple('f', atIf));
	// Whitespace after 'if'
	add(atIf, new Discard(new Whitespace(ifRoot)));
	// Otherwise, simple eval
	add(atIf, new Always(evalRoot, false));
	// Quotes inside if condition
	addStringLiteralParser(ifRoot, ifRoot, '\"', '\\');
	addStringLiteralParser(ifRoot, ifRoot, '\'', '\\');
	//Finish if
	add(ifRoot, new PymlAddIfTransition(&parser, new Skip(new Simple(':', start))));
	//Continue if condition
	add(ifRoot, new Always(ifRoot));

	// @else
	addBulkParser(atRoot2, elseRoot, evalRoot, "else");
	// Finish @else:
	add(elseRoot, new PymlAddElseTransition(&parser, new Simple(':', start)));
	// Optionally consume whitespace
	add(elseRoot, new Whitespace(elseRoot));
	// Otherwise, if's a simple eval
	add(elseRoot, new Always(evalRoot, false));

	// @/
	add(atRoot2, new Simple('/', atSlash));

	// @/if
	addBulkParser(atSlash, endIf, evalRoot, "if");
	// Finish /if with whitespace
	add(endIf, new PymlFinishIfTransition(&parser, new OrFinal(new Whitespace(start))));
	// Finish /if with @
	add(endIf, new PymlFinishIfTransition(&parser, new Skip(new Simple('@', start))));
	// Otherwise, simple eval
	add(endIf, new Always(evalRoot, false));

	// @for ... in ...:
	addBulkParser(atRoot2, atFor, evalRoot, "for");
	// Continue to set entry
	add(atFor, new Discard(new Skip(new Whitespace(forEntry))));
	// Otherwise, simple eval
	add(atFor, new Always(evalRoot, false));
	// Quotes setting entry (probably not really useful but you never know)
	addStringLiteralParser(forEntry, forEntry, '\"', '\\');
	addStringLiteralParser(forEntry, forEntry, '\'', '\\');
	// Whitespace before 'in'
	add(forEntry, new SavepointSet(new Whitespace(forPreIn)));
	// Continue for entry
	add(forEntry, new Always(forEntry));
	// actual "in"
	addBulkParser(forPreIn, forPostIn, forEntry, "in");
	// Otherwise, entry not finished.
	add(forPreIn, new Always(forEntry, false));
	// If whitespace after "in"
	add(forPostIn, new Push(new SavepointRevert(new Whitespace(forCollection))));
	// Otherwise, it wasn't a true "in"
	add(forPostIn, new Always(forEntry, false));
	// String literals in for collection
	addStringLiteralParser(forCollection, forCollection, '\"', '\\');
	addStringLiteralParser(forCollection, forCollection, '\'', '\\');
	// End of collection
	add(forCollection, new PymlAddForInTransition(&parser, new Push(new Skip(new Simple(':', start)))));
	// Continue for collection
	add(forCollection, new Always(forCollection));

	// /for
	addBulkParser(atSlash, endFor, evalRoot, "for");
	// Finish with whitespace
	add(endFor, new PymlFinishForTransition(&parser, new OrFinal(new Whitespace(start))));
	// Finish with @
	add(endFor, new PymlFinishForTransition(&parser, new Skip(new Simple('@', start))));
	// Otherwise, simple eval
	add(endFor, new Always(evalRoot, false));

	// @import
	addBulkParser(atI, atImport, evalRoot, "mport");
	// Whitespace after @import
	add(atImport, new Discard(new Skip(new Whitespace(importRoot))));
	// At import: quoted strings
	addStringLiteralParser(importRoot, importRoot, '\"', '\\');
	addStringLiteralParser(importRoot, importRoot, '\'', '\\');
	// Paranthesize import code to escape spaces
	addBlockParser(importRoot, importRoot, '(', ')');
	// Finish at import with whitespace
	add(importRoot, new PymlAddEmbedTransition(&parser, new OrFinal(new Whitespace(start))));
	// TODO: END ACTIONS: finalAddPymlStr - ???
	// Finish import with @
	add(importRoot, new PymlAddEmbedTransition(&parser, new Skip(new Simple('@', start))));
	// Continue @import
	add(importRoot, new Always(importRoot));

	// @import-ctrl
	// Start parser
	addBulkParser(atImport, atImportCtrl, evalRoot, "-ctrl");
	// Whitespace to mark importCtrl
	add(atImportCtrl, new Discard(new Skip(new Whitespace(importCtrlRoot))));
	// Otherwise simple eval
	add(atImportCtrl, new Always(evalRoot, false));
	// At import-ctrl
	addStringLiteralParser(importCtrlRoot, importCtrlRoot, '\"', '\\');
	addStringLiteralParser(importCtrlRoot, importCtrlRoot, '\'', '\\');
	// Paranthesize import-ctrl to escape spaces
	addBlockParser(importCtrlRoot, importCtrlRoot, '(', ')');
	// Finish import-ctrl with whitespace
	add(importCtrlRoot, new PymlAddCtrlTransition(&parser, new OrFinal(new Whitespace(start))));
	// Finish import-ctrl with @
	add(importCtrlRoot, new PymlAddEmbedTransition(&parser, new Skip(new Simple('@', start))));
	// Continue @import-ctrl
	add(importCtrlRoot, new Always(importCtrlRoot));


	// Add more @... here

	// Cleanup:
	// @i: if not "if" or "import", it's a simple eval
	add(atI, new Always(evalRoot, false));
	// @/: if not if or for, simple eval (it's probably going to fail, but the fail location may be more intuitive)
	add(atSlash, new Always(evalRoot, false));
	// not @import, not @import-ctrl => simple eval
	add(atImport, new Always(evalRoot, false));


	// @!expr
	add(atRoot2, new Skip(new Simple('!', evalRawRoot)));
	// Finish by whitespace
	add(evalRawRoot, new PymlAddPyCodeTransition(&parser,
	                                             new OrFinal(new Whitespace(start)), PymlWorkingItem::PyEvalRaw));
	// Finish by @...@
	add(evalRawRoot, new PymlAddPyCodeTransition(&parser,
	                                             new Skip(new Simple('@', start)), PymlWorkingItem::PyEvalRaw));
	//TODO: do ^^^ this for (most) instances of WhitespaceFsmTransition (including things like /if)
	// Escape strings
	addStringLiteralParser(evalRawRoot, evalRawRoot, '\"', '\\');
	addStringLiteralParser(evalRawRoot, evalRawRoot, '\'', '\\');
	// Paranthesize code to escape spaces
	addBlockParser(evalRawRoot, evalRawRoot, '(', ')');
	// Continue PyEvalRaw
	add(evalRawRoot, new Always(evalRawRoot));

	// @expr; THIS MUST BE THE LAST @... TRANSITIONS
	add(atRoot2, new Always(evalRoot, false));
	// Finish by whitespace
	add(evalRoot, new PymlAddPyCodeTransition(&parser, new OrFinal(new Whitespace(start)), PymlWorkingItem::PyEval));
	// Finish by @...@
	add(evalRoot, new PymlAddPyCodeTransition(&parser, new Skip(new Simple('@', start)), PymlWorkingItem::PyEval));
	// Escape strings
	addStringLiteralParser(evalRoot, evalRoot, '\"', '\\');
	addStringLiteralParser(evalRoot, evalRoot, '\'', '\\');
	// Paranthesize code to escape spaces
	addBlockParser(evalRoot, evalRoot, '(', ')');
	// Continue PyEvalRaw
	add(evalRoot, new Always(evalRoot));

	addFinalActionToMany([=](FsmV2& fsm) { finalAddPymlStr(); }, {
		                     start, atRoot, atRoot2
	                     });
	addFinalActionToMany([=](FsmV2& fsm) {
	                    finalThrowError(pymlError() <<
		                    stringInfo("Incomplete '@...'/'@!...' Python expression, most likely unfinished string literal"));
                    },
	                     {evalRoot, evalRawRoot});
	addFinalActionToMany([=](FsmV2& fsm) {
	                    finalThrowError(pymlError() <<
		                    stringInfo("Unterminated '@{ ... }' block."));
                    },
	                     {execRoot});
	addFinalActionToMany([=](FsmV2& fsm) {
	                    finalThrowError(pymlError() <<
		                    stringInfo("Incomplete '@if ... :' instruction."));
                    },
	                     {ifRoot});
	addFinalActionToMany([=](FsmV2& fsm) {
	                    finalThrowError(pymlError() <<
		                    stringInfo("Incomplete '@else:' instruction."));
                    },
	                     {elseRoot});
	addFinalActionToMany([=](FsmV2& fsm) {
	                    finalThrowError(pymlError() <<
		                    stringInfo("Incomplete '@for ... in ... :' instruction."));
                    },
	                     {forEntry, forPreIn, forPostIn, forCollection});
	addFinalActionToMany([=](FsmV2& fsm) {
	                    finalThrowError(pymlError() <<
		                    stringInfo("Incomplete '@import ...' instruction."));
                    },
	                     {importRoot});
	addFinalActionToMany([=](FsmV2& fsm) {
	                    BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat(
			                    "BUG IN PYML PARSER: stuck in final state %1%", fsm.getState()
		                    ));
                    },
	                     {atI, atIf, atFor, atSlash, endIf, endFor, atImport});
}

void V2PymlParserFsm::finalAddPymlStr() {
	std::string pymlStr = getResetStored();
	if (pymlStr.length() != 0) {
		parser->addPymlWorkingStr(pymlStr);
	}
}

void V2PymlParserFsm::finalThrowError(const pymlError& err) {
	BOOST_THROW_EXCEPTION(err);
}
