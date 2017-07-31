#include <memory>
#include "mvcPymlFile.h"
#include "pymlItems.h"

MvcPymlFile::MvcPymlFile(const boost::python::object& ctrlClass, IPymlCache& cache)
	: ctrlClass(ctrlClass), rootItem(nullptr), cache(cache) {
	setRootItem();
}

void MvcPymlFile::setRootItem() {
	//A bit tedious, but worth it. Still bad for python compile performance, needs tokenizing and stuff :|
	auto setCallable = std::make_unique<PymlItemSetCallable>(ctrlClass, "krait.mvc.init_ctrl");  //Warning, this may break if set_init_ctrl does something else!
	auto embedSetupExec = std::make_unique<PymlItemPyExec>("ctrl = krait.mvc.push_ctrl(krait.mvc.init_ctrl)");
	auto embedCleanupExec = std::make_unique<PymlItemPyExec>("ctrl = krait.mvc.pop_ctrl()");
	auto viewEmbed = std::make_unique<PymlItemEmbed>("krait.get_full_path(ctrl.get_view())", cache);
	rootItem = std::make_unique<PymlItemSeq>(std::vector<std::unique_ptr<const IPymlItem>>({
		std::move(setCallable),
		std::move(embedSetupExec),
		std::move(viewEmbed),
		std::move(embedCleanupExec)
	}));
}

bool MvcPymlFile::isDynamic() const {
	return rootItem->isDynamic();
}

std::string MvcPymlFile::runPyml() const {
	return rootItem->runPyml();
}

const IPymlItem* MvcPymlFile::getRootItem() const {
	return &*rootItem;
}
