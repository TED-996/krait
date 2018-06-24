#pragma once
#include <boost/utility/string_ref.hpp>
#include <ostream>
#include <string>
#include <vector>

class CodeAstItem {
    std::string code;
    std::vector<CodeAstItem> children;
    bool indentAfter;

public:
    explicit CodeAstItem() : CodeAstItem("", std::vector<CodeAstItem>(), false) {
    }
    explicit CodeAstItem(std::string&& code, bool indentAfter = false)
            : CodeAstItem(std::move(code), std::vector<CodeAstItem>(), indentAfter) {
    }
    explicit CodeAstItem(std::vector<CodeAstItem>&& codeAstItems) : CodeAstItem("", std::move(codeAstItems), false) {
    }

    CodeAstItem(std::string&& code, std::vector<CodeAstItem>&& codeAstItems, bool indentAfter = false);
    CodeAstItem(CodeAstItem&&) noexcept = default;
    CodeAstItem& operator=(CodeAstItem&&) noexcept;

    void addChild(CodeAstItem&& child);

    // Not strictly necessary, but EXPENSIVE. Maybe don't call.
    // Make sure not to call it before it's definitely a valid AST.
    CodeAstItem optimize();

    bool isValid() const;
    bool isEmpty() const;
    void addPassChildren(const std::string& code);

    size_t getCodeSize(size_t indentLevel) const;
    void getCodeToString(size_t indentLevel, std::string& destination) const;
    void getCodeToStream(size_t indentLevel, std::ostream& destination) const;

    // Important: Each part MUST be a separate thing. For example, a string, or something like that.
    // No newlines, except in multiline strings. No raw strings yet.
    // No guarantees if any of the requirements are not met.
    // As of now, only wraps strings.
    static CodeAstItem fromMultilineStatement(std::vector<boost::string_ref> parts);
    // Doesn't wrap stuff on its own. Only follows newlines and stuff.
    static CodeAstItem fromPythonCode(const std::string& code);
};
