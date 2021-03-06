#include "rawPythonPymlParser.h"
#include "pythonModule.h"

RawPythonPymlParser::RawPythonPymlParser(IPymlCache& cache)
	:
	cache(cache),
	mainExec(""),
	rootSeq(std::vector<const PymlItem*>()),
	ctrlCondition("False", nullptr, nullptr),
	embedRootSeq(std::vector<const PymlItem*>()),
	embedSetupExec(""),
	viewEmbed("", cache),
	embedCleanupExec("") {
}

void RawPythonPymlParser::consume(std::string::iterator start, std::string::iterator end) {
	//A bit tedious, but worth it.
	mainExec = PymlItemPyExec(PythonModule::prepareStr(std::string(start, end)));
	embedSetupExec = PymlItemPyExec("ctrl = krait.mvc.push_ctrl(krait.mvc.init_ctrl)");
	embedCleanupExec = PymlItemPyExec("ctrl = krait.mvc.pop_ctrl()");
	viewEmbed = PymlItemEmbed("krait.get_full_path(ctrl.get_view())", cache);
	embedRootSeq = PymlItemSeq(std::vector<const PymlItem*>({&embedSetupExec, &viewEmbed, &embedCleanupExec}));
	ctrlCondition = PymlItemIf("krait.mvc.init_ctrl is not None", &embedRootSeq, nullptr);
	rootSeq = PymlItemSeq(std::vector<const PymlItem*>({&mainExec, &ctrlCondition}));
}

const IPymlItem* RawPythonPymlParser::getParsed() {
	return &rootSeq;
}
