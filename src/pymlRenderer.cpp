#include "pymlRenderer.h"
#include "pymlIterator.h"


// ReSharper disable once CppMemberFunctionMayBeStatic
IteratorResult PymlRenderer::renderToIterator(const IPymlFile& pymlFile) const {
    return IteratorResult(PymlIterator(pymlFile.getRootItem()));
}

void PymlRenderer::renderToEmit(const IPymlFile& pymlFile) const {
    PymlIterator iterator(pymlFile.getRootItem());
    while ((*iterator).data() != nullptr) {
        boost::string_ref ref = *iterator;
        if (iterator.isTmpRef(ref)) {
            emitModule.emitStdString(ref.to_string());
        } else {
            // Is permanent reference. Must NOT be deallocated.
            emitModule.emitStringRef(ref);
        }
    }
}
