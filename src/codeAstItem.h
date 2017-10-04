#pragma once
#include <string>
#include <vector>

class CodeAstItem
{
	std::string code;
	std::vector<CodeAstItem> children;
	bool indentAfter;
public:
	explicit CodeAstItem(std::string&& code, bool indentAfter = false) :
		code(std::move(code)),
		children(),
		indentAfter(indentAfter) {
	}

	CodeAstItem(std::string&& code, std::vector<CodeAstItem>&& codeAstItems, bool indentAfter = false) :
		code(std::move(code)),
		children(std::move(codeAstItems)),
		indentAfter(indentAfter) {
	}

	CodeAstItem(CodeAstItem&&) = default;


	void addChild(CodeAstItem&& child) {
		children.emplace_back(std::move(child));
	}
	bool isValid() const;
	void addChildrenIfEmptyIndent(const std::string& code);

	size_t getCodeSize(size_t indentLevel) const;
	void getCode(size_t indentLevel, std::string& destination) const;
};
