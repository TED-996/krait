#include "pymlItems.h"

#include"except.h"
#include"utils.h"
#include"pythonModule.h"

#define DBG_DISABLE
#include "dbg.h"


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

std::string htmlEscape(std::string htmlCode);

std::string PymlItemPyEval::runPyml() const {
	return htmlEscape(PythonModule::main().eval(code));
}

std::string PymlItemPyEvalRaw::runPyml() const {
	return PythonModule::main().eval(code);
}


std::string PymlItemPyExec::runPyml() const {
	PythonModule::main().run(code);
	return "";
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


std::string PymlItemFor::runPyml() const {
	PythonModule::main().run(initCode);
	std::string result;
	while (PythonModule::main().test(conditionCode)) {
		result += loopItem->runPyml();
		PythonModule::main().run(updateCode);
	}
	return result;
}


const IPymlItem* PymlItemFor::getNext(const IPymlItem* last) const {
	if (last == nullptr) {
		PythonModule::main().run(initCode);
	}
	else {
		PythonModule::main().run(updateCode);
	}

	if (PythonModule::main().test(conditionCode)) {
		return loopItem.get();
	}
	else {
		return nullptr;
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
			 PythonModule::prepareStr(forData.initCode),
			 PythonModule::prepareStr(forData.conditionCode),
			 PythonModule::prepareStr(forData.updateCode),
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

std::string htmlEscape(std::string htmlCode) {
	std::string result;
	bool resultEmpty = true;

	const char* replacements[256];
	memzero(replacements);
	replacements[(int)'&'] = "&amp;";
	replacements[(int)'<'] = "&lt;";
	replacements[(int)'>'] = "&gt;";
	replacements[(int)'"'] = "&quot;";
	replacements[(int)'\''] = "&apos;";

	unsigned int oldIdx = 0;
	for (unsigned int idx = 0; idx < htmlCode.length(); idx++) {
		if (replacements[(int)htmlCode[idx]] != nullptr) {
			if (resultEmpty) {
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
