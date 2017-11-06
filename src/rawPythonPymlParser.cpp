#include "rawPythonPymlParser.h"
#include "pythonModule.h"

RawPythonPymlParser::RawPythonPymlParser(IPymlCache& cache)
        : cache(cache), rootSeq(std::make_unique<PymlItemSeq>(std::vector<std::unique_ptr<const IPymlItem>>())) {
}

void RawPythonPymlParser::consume(std::string::iterator start, std::string::iterator end) {
    // A bit tedious, but worth it.
    auto mainExec = std::make_unique<PymlItemPyExec>(PythonModule::prepareStr(std::string(start, end)));
    auto embedSetupExec = std::make_unique<PymlItemPyExec>("ctrl = krait.mvc.push_ctrl(krait.mvc.init_ctrl)");
    auto embedCleanupExec = std::make_unique<PymlItemPyExec>("ctrl = krait.mvc.pop_ctrl()");
    auto viewEmbed = std::make_unique<PymlItemEmbed>("krait.get_full_path(ctrl.get_view())", cache);

    std::vector<std::unique_ptr<const IPymlItem>> embedRootSeqVector;
    embedRootSeqVector.emplace_back(std::move(embedSetupExec));
    embedRootSeqVector.emplace_back(std::move(viewEmbed));
    embedRootSeqVector.emplace_back(std::move(embedCleanupExec));

    auto embedRootSeq = std::make_unique<PymlItemSeq>(std::move(embedRootSeqVector));
    auto ctrlCondition
        = std::make_unique<PymlItemIf>("krait.mvc.init_ctrl is not None", std::move(embedRootSeq), nullptr);

    std::vector<std::unique_ptr<const IPymlItem>> rootSeqVector;
    rootSeqVector.emplace_back(std::move(mainExec));
    rootSeqVector.emplace_back(std::move(ctrlCondition));

    rootSeq = std::make_unique<PymlItemSeq>(std::move(rootSeqVector));
}

std::unique_ptr<const IPymlItem> RawPythonPymlParser::getParsed() {
    if (rootSeq == nullptr) {
        BOOST_THROW_EXCEPTION(serverError() << stringInfo("Pyml parser: root null (double consumption)"));
    }
    return std::move(rootSeq);
}
