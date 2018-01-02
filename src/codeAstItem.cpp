#include "codeAstItem.h"
#include "except.h"
#include "pythonCodeEscapingFsm.h"

//#define DBG_DISABLE
#include "dbg.h"


struct Indent {
    const char* const data;
    const size_t size;
    const char chr;

    template<std::size_t N>
    explicit constexpr Indent(const char (&indent)[N]) : data(indent), size(N - 1), chr(getChr()) {
    }

private:
    constexpr char getChr(size_t idx = 0, char last = '\0') const {
        return idx == size
            ? last
            : idx == 0 ? getChr(1, data[0]) : data[idx] == last ? getChr(idx + 1, last) : throw std::exception();
    }
};

static constexpr Indent indent = Indent("    ");
static constexpr char newlineChar = '\n';


CodeAstItem::CodeAstItem(std::string&& code, std::vector<CodeAstItem>&& codeAstItems, bool indentAfter)
        : code(std::move(code)), children(std::move(codeAstItems)), indentAfter(indentAfter) {
    if (this->code.empty() && indentAfter) {
        BOOST_THROW_EXCEPTION(
            serverError() << stringInfo("CodeAstItem::CodeAstItem: empty code, contents expect indent."));
    }

    for (auto it = children.begin(); it != children.end();) {
        if (it->isEmpty()) {
            it = children.erase(it);
        } else {
            ++it;
        }
    }
}

CodeAstItem& CodeAstItem::operator=(CodeAstItem&& other) noexcept {
    code = std::move(other.code);
    children = std::move(other.children); // This doesn't throw, right? Anyway, it shouldn't.
    indentAfter = other.indentAfter;

    return *this;
}

void CodeAstItem::addChild(CodeAstItem&& child) {
    if (!child.isEmpty()) {
        children.emplace_back(std::move(child));
    }
}

CodeAstItem CodeAstItem::optimize() {
    // Unused code at the moment, also broken.
    // Also kind of useless, this should generate the same code before and after
    // and that's the only thing that matters.
    return std::move(*this);

    DBG("CodeAstItem::optimize");
    // If there is no code, and only one child, return it.
    /*if (code.empty() && children.size() == 1) {
        DBG("Applying one-child optimization");
        return children[0].optimize();
    }
    */

    if (!children.empty()) {
        DBG("Has children to transform");
        // Flatten all children.
        for (auto it = children.begin(); it != children.end(); ++it) {
            *it = it->optimize();
        }

        std::vector<CodeAstItem> newChildren;
        newChildren.reserve(children.size());
        for (auto& it : children) {
            if (it.code.empty() && it.children.empty()) {
                DBG("continue: Applying null child optimization");
                continue;
            }
            if (!it.code.empty() || it.indentAfter) {
                DBG("Adding child unchanged to list");
                newChildren.emplace_back(std::move(it));
            } else {
                DBG("Flattening children list");
                for (auto& itInner : it.children) {
                    newChildren.emplace_back(std::move(itInner));
                }
            }
        }
        children = std::move(newChildren);
    }

    DBG("return: Finished optimizing normally");
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
    // Indent, then code, then newline.
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

CodeAstItem CodeAstItem::fromMultilineStatement(std::vector<boost::string_ref> parts) {
    CodeAstItem result;
    constexpr size_t minWrapLength = 80;
    constexpr size_t acceptableWrapLength = 90;
    /* Findings:
    1. Wrapping doesn't hurt performance.
    2. You need '\' after each line if you wrap.
    3. You need to watch if you're in a string.
    */
    std::string line;
    line.reserve(minWrapLength);

    for (auto part : parts) {
        // First, make sure the line isn't already too long.
        if (line.length() > minWrapLength) {
            result.addChild(CodeAstItem(std::move(line)));
            line = "";
            line.reserve(minWrapLength);
        }

        enum class PartType { SingleLineCode, String, TripleQuoteString };

        PartType type;
        char termChar;

        // First, finding it.
        if (part.starts_with('\'') || part.starts_with('\"')) {
            type = PartType::String;
            termChar = part[0];
        } else if (part.starts_with("'''") || part.starts_with("\"\"\"")) {
            type = PartType::TripleQuoteString;
            termChar = part[0];
        } else {
            type = PartType::SingleLineCode;
            // No checking. Your job.

            // Silence some warnings.
            termChar = '\0';
        }

        // Then, wrapping it.
        if (type == PartType::SingleLineCode) {
            // We just add it.

            line.append(part.data(), part.length());
        } else if (type == PartType::String) {
            // We may need to do some wrapping.

            // If after adding it we're still under the limit; OR over, but not by much
            if (line.size() + part.size() < acceptableWrapLength) {
                // Just add it and continue.
                line.append(part.data(), part.size());

                // If it's over the limit, it will be appended at the start of the next loop.
            }
            // Otherwise, no easy way out.
            else {
                size_t lastIdx = 0;
                while (lastIdx < part.length()) {
                    // Compute the new line length.
                    size_t partOnLineLength =
                        // Maximum: the acceptable length
                        acceptableWrapLength -
                        // Reserve spot for the terminator
                        1 -
                        // Remove what we've already got
                        line.size();

                    // Cap the line.
                    size_t partLeftLength = part.length() - lastIdx;
                    if (partOnLineLength > partLeftLength) {
                        partOnLineLength = partLeftLength;
                    }

                    // We can't end on a backslash (or we can, but we need complicated counting.)
                    while (partOnLineLength < partLeftLength && partOnLineLength != 0
                        && part[lastIdx + partOnLineLength - 1] == '\\') {
                        partOnLineLength++;
                    }

                    // Don't get caught up in terminators and don't leave very short lines, it's more wasteful.
                    if (partOnLineLength < partLeftLength && partLeftLength - partOnLineLength <= 5) {
                        // Just give it all.
                        partOnLineLength = partLeftLength;
                    }

                    // Append this part (of a part).
                    line.append(part.data() + lastIdx, partOnLineLength);
                    // Append the terminator if it's not already there.
                    if (partOnLineLength != partLeftLength) {
                        line.append(1, termChar);
                        // Also append a backslash to signal a continuation line.
                        line.append(1, '\\');
                    }

                    // Add the line to the AST
                    result.addChild(CodeAstItem(std::move(line)));

                    // Update the index
                    lastIdx += partOnLineLength;

                    // Prepare the next line.
                    if (lastIdx < part.length()) {
                        line = std::string(1, termChar);
                    } else {
                        line = "";
                    }
                    line.reserve(acceptableWrapLength);
                }
            }
        } else {
            // PartType::TripleQuoteString
            // Wrap only, exactly, at newlines.
            size_t lastIdx = 0;
            size_t newlineIdx = part.find('\n');

            if (newlineIdx == std::string::npos) {
                line.append(part.data(), part.size());
            }

            while (newlineIdx != std::string::npos) {
                enum class NewlineStyle { Lf, CrLf };
                NewlineStyle style;
                // This is either LF or CRLF.
                if (newlineIdx != 0 && part[newlineIdx - 1] == '\r') {
                    style = NewlineStyle::CrLf;
                } else {
                    style = NewlineStyle::Lf;
                }

                // If it's CRLF, don't copy the CR.
                size_t appendPartSize = newlineIdx - lastIdx;
                if (style == NewlineStyle::CrLf) {
                    appendPartSize--;
                }

                // Add the line contents.
                line.append(part.data() + lastIdx, appendPartSize);
                // Add the CRLF/LF, escaped.
                if (style == NewlineStyle::CrLf) {
                    line.append("\\r\\n", 4);
                } else {
                    line.append("\\n", 2);
                }
                // Add the line terminator
                line.append(3, termChar);

                size_t nextIdx = newlineIdx + 1;
                // If the string ends right after the newlines, don't make a new string.
                if (nextIdx == part.size() - 3) {
                    // The terminator has already been added. Just update the next index.
                    nextIdx = part.size();
                }

                if (nextIdx != part.size()) {
                    line.append(1, '\\');
                }
            }
            if (lastIdx < part.size()) {
                // The end already contains the terminator, and the line already contains the start
                // (quote characters and possibly newlines).
                line.append(part.data() + lastIdx, part.size() - lastIdx);
            }
        }
    }

    if (line.length() > 0) {
        result.addChild(CodeAstItem(std::move(line)));
    }

    return result;
}

CodeAstItem CodeAstItem::fromPythonCode(const std::string& code) {
    static PythonCodeEscapingFsm escapingFsm;
    escapingFsm.reset();

    // Split on newlines.
    CodeAstItem result;

    size_t lastIdx = 0;
    char tripleQuoteAtLineStart = '\0';
    while (lastIdx < code.size()) {
        size_t newlineIdx = code.find('\n', lastIdx);
        size_t lineEnd = newlineIdx;
        size_t nextIdx = newlineIdx + 1;
        size_t newlineStart = newlineIdx;

        if (newlineIdx == std::string::npos) {
            lineEnd = code.size();
            nextIdx = code.size();
            newlineStart = code.size();
        }

        // If the newline style is CRLF, don't copy the CR to the code.
        if (lineEnd != code.size() && lineEnd != 0 && code[lineEnd - 1] == '\r') {
            lineEnd--;
            newlineStart--;
        }

        for (auto it = code.cbegin() + lastIdx; it != code.cbegin() + nextIdx; ++it) {
            escapingFsm.consumeOne(*it);
        }

        std::string line = code.substr(lastIdx, lineEnd - lastIdx);
        if (tripleQuoteAtLineStart != '\0') {
            line = std::string(3, tripleQuoteAtLineStart) + line;
            tripleQuoteAtLineStart = '\0';
        }
        if (escapingFsm.isMultilineString()) {
            tripleQuoteAtLineStart = escapingFsm.getMultilineQuoteChar();

            // Append newlines.
            std::string newlines = "";
            if (newlineStart != nextIdx) {
                newlines = "\"";
                newlines.reserve(6);
                for (auto it = code.cbegin() + newlineStart; it != code.cbegin() + nextIdx; ++it) {
                    if (*it == '\r') {
                        newlines.append("\\r", 2);
                    } else if (*it == '\n') {
                        newlines.append("\\n", 2);
                    } else {
                        BOOST_THROW_EXCEPTION(serverError()
                            << stringInfoFromFormat("Unexpected character 0x%02X in newline string.", (uint8_t) *it));
                    }
                }
            }

            line += std::string(3, tripleQuoteAtLineStart);
            line += newlines;
        }

        result.addChild(CodeAstItem(std::move(line)));
        lastIdx = nextIdx;
    }

    return result;
}
