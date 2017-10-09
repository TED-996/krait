#include "pymlItems.h"

#include"except.h"
#include"utils.h"
#include"pythonModule.h"

#define DBG_DISABLE
#include "dbg.h"


// Implemented in utils.cpp
std::string reprPythonString(const std::string& str);


CodeAstItem PymlItemStr::getCodeAst() const {
	std::string strRepr = reprPythonString(str);
	return CodeAstItem::fromMultilineStatement(std::vector<boost::string_ref>{
		"krait._emit_raw(",
		strRepr,
		")"});
}

std::string PymlItemSeq::runPyml() const {
	std::string result;
	for (const auto& it : items) {
		result += it->runPyml();
	}
	return result;
}

bool PymlItemSeq::isDynamic() const {
	for (const auto& it: items) {
		if (it->isDynamic()) {
			return true;
		}
	}
	return false;
}

const IPymlItem* PymlItemSeq::getNext(const IPymlItem* last) const {
	if (last == nullptr && items.size() != 0) {
		return items[0].get();
	}

	for (int i = 0; i < (int)items.size(); i++) {
		if (items[i].get() == last && i + 1 < (int)items.size()) {
			return items[i + 1].get();
		}
	}

	return nullptr;
}

CodeAstItem PymlItemSeq::getCodeAst() const {
	CodeAstItem result("");
	for (const auto& it : items) {
		result.addChild(it->getCodeAst());
	}
	return result;
}

bool PymlItemSeq::canConvertToCode() const {
	for (const auto& it : items) {
		if(!it->canConvertToCode()) {
			return false;
		}
	}
	return true;
}

std::unique_ptr<CodeAstItem> PymlItemSeq::getHeaderAst() const {
	std::unique_ptr<CodeAstItem> headers = std::make_unique<CodeAstItem>();
	for (const auto& it : items) {
		std::unique_ptr<CodeAstItem> childHeader = it->getHeaderAst();
		if (childHeader != nullptr) {
			headers->addChild(std::move(*childHeader));
		}
	}
	if (headers->isEmpty()) {
		headers = nullptr;
	}

	//Doing this to allow for copy elision.
	return headers;
}

std::string PymlItemPyEval::runPyml() const {
	std::string result = PythonModule::main().eval(code);
	boost::optional<std::string> escaped = htmlEscapeRef(result);
	
	//Doing this instead of something more straight-forward to allow for copy elision.
	if (escaped != boost::none) {
		result = std::move(escaped.get());
	}

	return result;

}


CodeAstItem PymlItemPyEval::getCodeAst() const {
	return CodeAstItem(std::string("krait._emit(") + code + ")");
}

std::string PymlItemPyEvalRaw::runPyml() const {
	return PythonModule::main().eval(code);
}

CodeAstItem PymlItemPyEvalRaw::getCodeAst() const {
	return CodeAstItem(std::string("krait._emit_raw(") + code + ")");
}

std::string PymlItemPyExec::runPyml() const {
	PythonModule::main().run(code);
	return "";
}

CodeAstItem PymlItemPyExec::getCodeAst() const {
	return CodeAstItem::fromPythonCode(code);
}

std::string PymlItemIf::runPyml() const {
	if (PythonModule::main().test(conditionCode)) {
		if (itemIfTrue == nullptr) {
			return "";
		}
		return itemIfTrue->runPyml();
	}
	else {
		if (itemIfFalse == nullptr) {
			return "";
		}
		return itemIfFalse->runPyml();
	}
}


const IPymlItem* PymlItemIf::getNext(const IPymlItem* last) const {
	if (last != nullptr) {
		return nullptr;
	}

	if (PythonModule::main().test(conditionCode)) {
		return itemIfTrue.get();
	}
	else {
		return itemIfFalse.get();
	}
}


CodeAstItem PymlItemIf::getCodeAst() const {
	CodeAstItem rootItem;
	CodeAstItem ifItem("if " + conditionCode + ":", true);
	if (itemIfTrue != nullptr) {
		ifItem.addChild(itemIfTrue->getCodeAst());
	}
	else {
		ifItem.addChild(CodeAstItem("pass"));
	}
	if (itemIfFalse == nullptr) {
		rootItem = std::move(ifItem);
	}
	else {
		rootItem.addChild(std::move(ifItem));
		
		CodeAstItem elseItem("else", true);
		elseItem.addChild(itemIfFalse->getCodeAst());
		
		rootItem.addChild(std::move(elseItem));
	}

	return rootItem;
}

bool PymlItemIf::canConvertToCode() const {
	if (itemIfTrue != nullptr && !itemIfTrue->canConvertToCode()) {
		return false;
	}
	if (itemIfFalse != nullptr && !itemIfFalse->canConvertToCode()) {
		return false;
	}
	return true;
}

std::unique_ptr<CodeAstItem> PymlItemIf::getHeaderAst() const {
	std::unique_ptr<CodeAstItem> headers = std::make_unique<CodeAstItem>();

	if (itemIfTrue != nullptr) {
		std::unique_ptr<CodeAstItem> childHeader = itemIfTrue->getHeaderAst();
		if (childHeader != nullptr) {
			headers->addChild(std::move(*childHeader));
		}
	}
	if (itemIfFalse != nullptr) {
		std::unique_ptr<CodeAstItem> childHeader = itemIfFalse->getHeaderAst();
		if (childHeader != nullptr) {
			headers->addChild(std::move(*childHeader));
		}
	}

	if (headers->isEmpty()) {
		headers = nullptr;
	}

	//Doing this to allow for copy elision.
	return headers;
}

std::string PymlItemFor::runPyml() const {
	PythonModule::main().run(initCode);

	std::string result;

	while (PythonModule::main().test(condCode)) {
		result += loopItem->runPyml();
		PythonModule::main().run(updateCode);
	}
	PythonModule::main().run(cleanupCode);

	return result;
}


const IPymlItem* PymlItemFor::getNext(const IPymlItem* last) const {
	if (last == nullptr) {
		PythonModule::main().run(initCode);
	}
	else {
		PythonModule::main().run(updateCode);
	}

	if (PythonModule::main().test(condCode)) {
		return loopItem.get();
	}
	else {
		PythonModule::main().run(cleanupCode);
		return nullptr;
	}
}


CodeAstItem PymlItemFor::getCodeAst() const {
	return CodeAstItem(
		formatString("for %1% in %2%:", entryName, collection),
		std::vector<CodeAstItem>{loopItem->getCodeAst()},
		true);
}

bool PymlItemFor::canConvertToCode() const {
	return loopItem == nullptr || loopItem->canConvertToCode();
}

std::unique_ptr<CodeAstItem> PymlItemFor::getHeaderAst() const {
	if (loopItem == nullptr) {
		return nullptr;
	}
	else {
		return loopItem->getHeaderAst();
	}
}

const IPymlItem* PymlItemEmbed::getNext(const IPymlItem* last) const {
	if (last == nullptr) {
		return cache.get(PythonModule::main().eval(filename)).getRootItem();
	}
	else {
		return nullptr;
	}
}

std::string PymlItemEmbed::runPyml() const {
	return cache.get(PythonModule::main().eval(filename)).runPyml();
}

bool PymlItemEmbed::isDynamic() const {
	return cache.get(PythonModule::main().eval(filename)).isDynamic();
}


boost::optional<std::string> getStaticPython(std::string code);

CodeAstItem PymlItemEmbed::getCodeAst() const {
	//TODO: use __import__? Use Compiler? Idk...
}

std::unique_ptr<CodeAstItem> PymlItemEmbed::getHeaderAst() const {
	//TODO: idk, what to import? Since the import is dynamic... Check if it actually is static though?
	//TODO FOR REALZ IMPORTANT: signal dependencies! OOOR use import hooks...
	boost::optional<std::string> staticModule = getStaticPython(filename);
	if (staticModule == boost::none) {
		// The import is not statically resolvable.
		return nullptr;
	}
}

bool PymlItemEmbed::canConvertToCode() const {
	//TODO: idk, check from Compiler cache if it's okay...
	return false;
	// Make all code with imports uncompilable.
}

std::string PymlItemSetCallable::runPyml() const {
	std::string tempName = "__kr_int_" + randomAlpha(32);

	PythonModule::main().setGlobal(tempName, PythonModule::main().callObject(callable));
	PythonModule::main().run(formatString("%1% = %2%", destination, tempName));
	PythonModule::main().run(formatString("del %1%", tempName));

	return "";
}

PymlWorkingItem::PymlWorkingItem(PymlWorkingItem::Type type)
	: data(NoneData()) {
	this->type = type;
	if (type == Type::None) {
	}
	else if (type == Type::Str) {
		data = StrData();
	}
	else if (type == Type::Seq) {
		data = SeqData();
	}
	else if (type == Type::PyEval || type == Type::PyEvalRaw || type == Type::PyExec) {
		PyCodeData dataTmp;
		dataTmp.type = type;
		data = dataTmp;
	}
	else if (type == Type::If) {
		data = IfData();
	}
	else if (type == Type::For) {
		data = ForData();
	}
	else if (type == Type::Embed) {
		data = EmbedData();
	}
	else {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Server Error parsing pyml file: PymlWorkingItem type not recognized."));
	}
}


class GetItemVisitor : public boost::static_visitor<std::unique_ptr<const IPymlItem>>
{
public:
	GetItemVisitor() = default;

	std::unique_ptr<const IPymlItem> operator()(PymlWorkingItem::NoneData data) {
		(void)data; //Silence the warning.
		return std::make_unique<const PymlItem>();
	}

	std::unique_ptr<const IPymlItem> operator()(PymlWorkingItem::StrData strData) {
		return std::make_unique<const PymlItemStr>(strData.str);
	}

	std::unique_ptr<const IPymlItem> operator()(PymlWorkingItem::SeqData seqData) {
		std::vector<std::unique_ptr<const IPymlItem>> items;
		for (PymlWorkingItem* it : seqData.items) {
			std::unique_ptr<const IPymlItem> item = it->getItem();
			if (item != nullptr) {
				items.push_back(std::move(item));
			}
		}

		if (items.size() == 0) {
			return nullptr;
		}
		if (items.size() == 1) {
			return std::unique_ptr<const IPymlItem>(std::move(items[0]));
		}
		return std::make_unique<const PymlItemSeq>(std::move(items));
	}

	std::unique_ptr<const IPymlItem> operator()(PymlWorkingItem::PyCodeData pyCodeData) {
		if (pyCodeData.type == PymlWorkingItem::Type::PyExec) {
			return std::make_unique<const PymlItemPyExec>(PythonModule::prepareStr(pyCodeData.code));
		}
		if (pyCodeData.type == PymlWorkingItem::Type::PyEval) {
			return std::make_unique<const PymlItemPyEval>(PythonModule::prepareStr(pyCodeData.code));
		}
		if (pyCodeData.type == PymlWorkingItem::Type::PyEvalRaw) {
			return std::make_unique<const PymlItemPyEvalRaw>(PythonModule::prepareStr(pyCodeData.code));
		}
		BOOST_THROW_EXCEPTION(
			serverError()
			<< stringInfo("Error parsing pyml file: Unrecognized type in PymlWorkingItem::PyCodeData."));
	}

	std::unique_ptr<const IPymlItem> operator()(PymlWorkingItem::IfData ifData) {
		std::unique_ptr<const IPymlItem> itemIfTrue = ifData.itemIfTrue->getItem();
		std::unique_ptr<const IPymlItem> itemIfFalse = nullptr;
		if (ifData.itemIfFalse != nullptr) {
			itemIfFalse = ifData.itemIfFalse->getItem();
		}

		return std::make_unique<PymlItemIf>(PythonModule::prepareStr(ifData.condition), std::move(itemIfTrue), std::move(itemIfFalse));
	}

	std::unique_ptr<const IPymlItem> operator()(PymlWorkingItem::ForData forData) {
		std::unique_ptr<const IPymlItem> loopItem = forData.loopItem->getItem();
		 
		return std::make_unique<const PymlItemFor>(
			forData.entryName,
			forData.collection,
			std::move(loopItem));
	}

	std::unique_ptr<const IPymlItem> operator()(PymlWorkingItem::EmbedData embedData) {
		return std::make_unique<PymlItemEmbed>(PythonModule::prepareStr(embedData.filename), *embedData.cache);
	}
};

std::unique_ptr<const IPymlItem> PymlWorkingItem::getItem() const {
	GetItemVisitor visitor;
	return boost::apply_visitor(visitor, data);
}
