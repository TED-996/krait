#include "moduleCompiler.h"
#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <unordered_map>
#include <unordered_set>


#define DBG_DISABLE
#include "dbg.h"


static std::vector<std::string> getEscapeFilenameReplacements();
static std::unordered_set<std::string> getReservedKeywords();


ModuleCompiler::ModuleCompiler(boost::filesystem::path siteRoot) : siteRoot(std::move(siteRoot)) {
}

void ModuleCompiler::compile(const IPymlFile& pymlFile, const std::string& destFilename, std::string&& cacheTag) {
    const IPymlItem* rootItem = pymlFile.getRootItem();

    if (!rootItem->canConvertToCode()) {
        BOOST_THROW_EXCEPTION(compileError() << stringInfo("Tried compiling non-compilable PymlFile"));
    }

    std::unique_ptr<CodeAstItem> customHeader = nullptr;
    CodeAstItem codeAst = rootItem->getCodeAst();
    codeAst.addPassChildren("pass");

    // codeAst.optimize();

    std::ofstream outFile(destFilename);
    if (!outFile) {
        BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Cannot open file %1% to compile", destFilename));
    }

    // Strip nulls from std::string.
    outFile << formatString(R"""(# [TAGS]: {"c_version": 1, "etag": "%1%"})""", cacheTag.c_str());
    outFile << "\n";

    static CodeAstItem moduleHeader = getModuleHeader();
    moduleHeader.getCodeToStream(0, outFile);

    if (customHeader != nullptr) {
        customHeader->addPassChildren("pass");
        customHeader->optimize();
        customHeader->getCodeToStream(0, outFile);
    }
    customHeader.reset();

    outFile << "\n\ndef run():\n";

    static CodeAstItem functionHeader = getFunctionHeader();
    functionHeader.getCodeToStream(1, outFile);

    codeAst.getCodeToStream(1, outFile);
}

CodeAstItem ModuleCompiler::getFunctionHeader() {
    CodeAstItem result;
    result.addChild(CodeAstItem("__emit__ = __internal._emit"));
    result.addChild(CodeAstItem("__emit_raw__ = __internal._emit_raw"));
    result.addChild(CodeAstItem("__run__ = __internal._compiled_run"));
    result.addChild(CodeAstItem("__to_module__ = __internal._compiled_convert_filename"));
    result.addChild(CodeAstItem("if hasattr(__loader__, 'check_tag_or_reload'): __loader__.check_tag_or_reload()"));
    result.addChild(CodeAstItem("ctrl = __mvc.curr_ctrl"));

    return result;
}

CodeAstItem ModuleCompiler::getModuleHeader() {
    CodeAstItem result;
    result.addChild(CodeAstItem("from __future__ import absolute_import"));
    result.addChild(CodeAstItem("import krait"));
    result.addChild(CodeAstItem("from krait import __internal"));
    result.addChild(CodeAstItem("from krait import mvc as __mvc"));

    return result;
}

// Convert an absolute/relative filename to its Python module name
std::string ModuleCompiler::escapeFilename(boost::string_ref filename) const {
    std::string result;
    std::string relativeFilename
        = boost::filesystem::relative(boost::filesystem::path(filename.data()), siteRoot).string();
    filename = relativeFilename;

    // We must escape EVERYTHING except alphanumeric characters
    // Also, we cannot start with a digit, and we cannot use Python keywords (to be checked later)

    static std::string moduleHeader = "_krait_compiled.";
    static std::vector<std::string> replacements = getEscapeFilenameReplacements();
    static std::unordered_set<std::string> reservedKeywords = getReservedKeywords();

    // Approximate the filename size at the cost of some memory usage.
    result.reserve(filename.size() + filename.size() / 6 + moduleHeader.size());

    size_t oldIdx = 0;
    for (size_t idx = 0; idx < filename.size(); idx++) {
        if (replacements[(int) filename[idx]].size() != 0) {
            // We have to convert it.
            result.append(filename.data() + oldIdx, idx - oldIdx);
            result.append(replacements[(int) filename[idx]]);
            oldIdx = idx + 1;
        }
    }
    result.append(filename.data() + oldIdx, filename.length() - oldIdx);

    if (result[0] >= '0' && result[0] <= '9') {
        // This converts from, say, 5_libraries.js to _35_libraries.js
        // which helps because 0x3X is exactly the digit X in ASCII
        result.insert(0, "_3", 2);
    }

    if (reservedKeywords.find(result) != reservedKeywords.end()) {
        // Is a reseved keyword, change the last character to its hex escape.
        // No reserved keywords contain any sort of escapes, so we can always do that.
        char lastCh = result[result.size() - 1];
        result[result.size() - 1] = '_';
        result.append(formatString("%02X", lastCh));
    }

    result.insert(0, moduleHeader);

    return result;
}

std::vector<std::string> getEscapeFilenameReplacements() {
    std::vector<std::string> replacements;
    replacements.reserve(256);

    /* Escaping:
     * '_' => "__" (underscore)
     * '/' => '_s' (Slash)
     * '\' => '_i' (backsIash)
     * '.' => '_p' (Period)
     * '~' => '_t' (Tilde)
     * ' ' => '_r' (spaceR)
     * '?' => '_q' (Question)
     * ':' => '_h' (coHlon)
     * '*' => '_n' (Nmultiply)
     * '+' => '_l' (pLus)
     * other => '_xx' where xx is hex for that character's ASCII
     */

    // Push defaults
    for (size_t i = 0; i < 256; i++) {
        if (std::isalnum(i)) {
            replacements.push_back(std::string());
        } else {
            replacements.push_back(formatString("_%02x", i));
        }
    }

    // Change specifics
    // These don't all make sense; letters must not collide with the hex digits
    // Also must not collide with _mvc_compiled
    replacements[(size_t) '_'] = "__";
    replacements[(size_t) '/'] = "_s";
    replacements[(size_t) '\\'] = "_i";
    replacements[(size_t) '.'] = "_p";
    replacements[(size_t) '~'] = "_t";
    replacements[(size_t) ' '] = "_r";
    replacements[(size_t) '?'] = "_q";
    replacements[(size_t) ':'] = "_h";
    replacements[(size_t) '*'] = "_n";
    replacements[(size_t) '+'] = "_l";

    return replacements;
}

std::unordered_set<std::string> getReservedKeywords() {
    return std::unordered_set<std::string>{"and",
        "del",
        "from",
        "not",
        "while",
        "as",
        "elif",
        "global",
        "or",
        "with",
        "assert",
        "else",
        "if",
        "pass",
        "yield",
        "break",
        "except",
        "import",
        "print",
        "class",
        "exec",
        "in",
        "raise",
        "continue",
        "finally",
        "is",
        "return",
        "def",
        "for",
        "lambda",
        "try"};
}

static bool is_hex_digit(char ch) {
    return (ch >= '0' && ch <= '9') || ((ch | 0x20) >= 'a' && (ch | 0x20) <= 'f');
}
static int to_hex_digit(char ch) {
    // Precondition: This is a hex digit (uppercase or lowercase)
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    return (ch | 0x20) - 'a' + 10;
}

static std::vector<char> getUnescapeFilenameReplacements();

// Returns an ABSOLUTE path from a module name. This should be the filename of the source file.
std::string ModuleCompiler::unescapeFilename(boost::string_ref moduleName) const {
    std::string result;
    result.reserve(moduleName.length());

    static std::vector<char> replacements = getUnescapeFilenameReplacements();

    if (moduleName.starts_with("_krait_compiled.")) {
        moduleName.remove_prefix(std::strlen("_krait_compiled."));
    }

    auto lastIt = moduleName.begin();
    for (auto it = moduleName.begin(); it != moduleName.end(); ++it) {
        if (*it == '_') {
            result.append(lastIt, it);

            ++it;
            if (is_hex_digit(*it)) {
                // This is a hex escape (_xx)
                int number = to_hex_digit(*it) << 4;
                ++it;
                if (!is_hex_digit(*it)) {
                    DBG_FMT("ERROR: Bad hex digit '_%1%' in '%2%'", *it, moduleName);
                    BOOST_THROW_EXCEPTION(serverError()
                        << stringInfo("Bad compiled module name to unescape: unfinished hex ecape sequence"));
                }
                result.append(1, (signed char) (number | to_hex_digit(*it)));
            } else if (replacements[*it] == '\0') {
                DBG_FMT("ERROR: Cannot replace '_%1%' in '%2%'", *it, moduleName);
                BOOST_THROW_EXCEPTION(
                    serverError() << stringInfo("Bad compiled module name to unescape: unrecognized escape character"));
            } else {
                result.append(1, replacements[(size_t) *it]);
            }
            lastIt = it + 1;
        }
    }

    result.append(lastIt, moduleName.end());

    result = (siteRoot / result).string();
    return result;
}

std::vector<char> getUnescapeFilenameReplacements() {
    std::vector<std::string> forwardReplacements = getEscapeFilenameReplacements();
    std::vector<char> result;
    result.resize(256, 0);

    for (size_t i = 0; i < forwardReplacements.size(); i++) {
        if (forwardReplacements[i].size() == 2) {
            char escapedChar = forwardReplacements[i][1];
            result[escapedChar] = (char) i;
        }
    }

    return result;
}
