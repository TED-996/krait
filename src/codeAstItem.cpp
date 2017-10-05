#include "codeAstItem.h"
#include "except.h"


struct Indent
{
	const const char* data;
	const size_t size;
	const char chr;

	template<std::size_t N>
	explicit constexpr Indent(const char(&indent)[N]) : data(indent), size(N - 1), chr(getChr()) {}

private:
	constexpr char getChr(size_t idx = 0, char last = '\0') const {
		return idx == size ? last :
			idx == 0 ? getChr(1, data[0]) :
			data[idx] == last ? getChr(idx + 1, last) :
				throw std::exception();
	}
};

static constexpr Indent indent = Indent("    ");
static constexpr char newlineChar = '\n';


CodeAstItem::CodeAstItem(std::string&& code, std::vector<CodeAstItem>&& codeAstItems, bool indentAfter)
	:
	code(std::move(code)),
	children(std::move(codeAstItems)),
	indentAfter(indentAfter) {
	
	if (this->code.empty() && indentAfter) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("CodeAstItem::CodeAstItem: empty code, contents expect indent."));
	}

	for (auto it = children.begin(); it != children.end();) {
		if (it->isEmpty()) {
			it = children.erase(it);
		}
		else {
			++it;
		}
	}
}

void CodeAstItem::addChild(CodeAstItem&& child) {
	if (!child.isEmpty()) {
		children.emplace_back(std::move(child));
	}
}

CodeAstItem CodeAstItem::flatten() {
	// If there is no code, and only one child, return it.
	if (code.empty() && children.empty()) {
		return children[0].optimize();
	}

	// Flatten all children.
	for (auto it = children.begin(); it != children.end(); ++it) {
		*it = std::move(it->optimize());
	}
	return std::move(*this);
}

bool CodeAstItem::isValid() const {
	if (indentAfter && children.empty()) {
		return false;
	}

	for (const CodeAstItem& child : children) {
		if (!child.isValid()) {
			return false;
		}
	}
	return true;
}

bool CodeAstItem::isEmpty() const {
	return code.empty() && children.empty();
}

void CodeAstItem::addPassChildren(const std::string& passCode) {
	if (indentAfter && children.size() == 0) {
		addChild(CodeAstItem(std::string(passCode)));
	}

	for (CodeAstItem& child : children) {
		child.addPassChildren(passCode);
	}
}

size_t CodeAstItem::getCodeSize(size_t indentLevel) const {
	//Indent, then code, then newline.
	size_t sum = 0;

	if (code.length() != 0) {
		sum += indentLevel * indent.size + code.length() + 1;
	}

	if (indentAfter) {
		indentLevel++;
	}

	for (const CodeAstItem& child : children) {
		sum += child.getCodeSize(indentLevel);
	}

	return sum;
}

void CodeAstItem::getCodeToString(size_t indentLevel, std::string& destination) const {
	if (code.length() != 0) {
		destination.append(indent.size * indentLevel, indent.chr);
		destination.append(code);
		destination.append(1, newlineChar);
	}

	if (indentAfter) {
		indentLevel++;
	}

	for (const CodeAstItem& child : children) {
		child.getCodeToString(indentLevel, destination);
	}
}

void CodeAstItem::getCodeToStream(size_t indentLevel, std::ostream& destination) const {
	if (code.length() != 0) {
		for (int i = 0; i < indentLevel; i++) {
			destination << indent.data;
		}
		destination << code;
		destination << newlineChar;
	}

	if (indentAfter) {
		indentLevel++;
	}

	for (const CodeAstItem& child : children) {
		child.getCodeToStream(indentLevel, destination);
	}
}
