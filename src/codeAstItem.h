#pragma once
#include <string>
#include <vector>
#include <ostream>

class CodeAstItem
{
	std::string code;
	std::vector<CodeAstItem> children;
	bool indentAfter;
public:

	explicit CodeAstItem() : CodeAstItem("", std::vector<CodeAstItem>(), false) {
	}
	explicit CodeAstItem(std::string&& code, bool indentAfter = false) : CodeAstItem(std::move(code), std::vector<CodeAstItem>(), indentAfter) {
	}
	explicit CodeAstItem(std::vector<CodeAstItem>&& codeAstItems) : CodeAstItem("", std::move(codeAstItems), false) {
	}

	CodeAstItem(std::string&& code, std::vector<CodeAstItem>&& codeAstItems, bool indentAfter = false);
	CodeAstItem(CodeAstItem&&) noexcept = default;
	CodeAstItem& operator=(CodeAstItem&&) = default;
	
	void addChild(CodeAstItem&& child);

	// Not strictly necessary. Maybe don't call.
	CodeAstItem optimize();
	// #brutal: Optimizes the AST by taking the children of a simple sequence of AST items.
	bool takeChildren(CodeAstItem&& source);

	bool isValid() const;
	bool isEmpty() const;
	void addPassChildren(const std::string& code);

	size_t getCodeSize(size_t indentLevel) const;
	void getCodeToString(size_t indentLevel, std::string& destination) const;
	void getCodeToStream(size_t indentLevel, std::ostream& destination) const;
};
