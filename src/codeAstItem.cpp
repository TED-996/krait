#include "codeAstItem.h"

static const size_t indentChars = 4;
static const char indentChar = ' ';
static const char newlineChar = '\n';


bool CodeAstItem::isValid() const {
	if (indentAfter && children.size() == 0) {
		return false;
	}

	for (const CodeAstItem& child : children) {
		if (!child.isValid()) {
			return false;
		}
	}
	return true;
}

void CodeAstItem::addChildrenIfEmptyIndent(const std::string& code) {
	if (indentAfter && children.size() == 0) {
		addChild(CodeAstItem(std::string(code)));
	}

	for (CodeAstItem& child : children) {
		child.addChildrenIfEmptyIndent(code);
	}
}

size_t CodeAstItem::getCodeSize(size_t indentLevel) const {
	//Indent, then code, then newline.
	size_t sum = indentLevel * indentChars + code.length() + 1;

	if (indentAfter) {
		indentLevel++;
	}

	for (const CodeAstItem& child : children) {
		sum += child.getCodeSize(indentLevel);
	}

	return sum;
}

void CodeAstItem::getCode(size_t indentLevel, std::string& destination) const {
	destination.append(indentChars * indentLevel, indentChar);
	destination.append(code);
	destination.append(1, newlineChar);

	if (indentAfter) {
		indentLevel++;
	}

	for (const CodeAstItem& child : children) {
		child.getCode(indentLevel, destination);
	}
}
