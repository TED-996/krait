#include <memory>
#include "mvcPymlFile.h"
#include "pymlItems.h"

MvcPymlFile::MvcPymlFile(const boost::python::object& ctrlClass, IPymlCache& cache)
        : ctrlClass(ctrlClass), rootItem(nullptr), cache(cache) {
    setRootItem();
}

void MvcPymlFile::setRootItem() {
    // A bit tedious, but worth it. Still bad for python compile performance, needs tokenizing and stuff :|

    std::vector<std::unique_ptr<const IPymlItem>> rootSeq;

    // Warning, this may break if set_init_ctrl does something else!
    rootSeq.push_back(std::make_unique<PymlItemSetCallable>(ctrlClass, "krait.mvc.init_ctrl"));
    rootSeq.push_back(std::make_unique<PymlItemPyExec>("ctrl = krait.mvc.push_ctrl(krait.mvc.init_ctrl)"));
    rootSeq.push_back(std::make_unique<PymlItemEmbed>("krait.get_full_path(ctrl.get_view())", cache));
    rootSeq.push_back(std::make_unique<PymlItemPyExec>("ctrl = krait.mvc.pop_ctrl()"));

    rootItem = std::make_unique<PymlItemSeq>(std::move(rootSeq));
}


std::string MvcPymlFile::runPyml() const {
    return rootItem->runPyml();
}
