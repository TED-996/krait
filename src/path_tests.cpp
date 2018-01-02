#include <boost/test/unit_test.hpp>
#include <iostream>

#include "path.h"

using namespace boost::filesystem;
using namespace std;

#include <boost/static_assert.hpp>
BOOST_STATIC_ASSERT(BOOST_VERSION >= 106200);


BOOST_AUTO_TEST_SUITE(mod_Path)

BOOST_AUTO_TEST_CASE(func_getExecRoot) {
    cout << "\nTesting getExecRoot:\n";

    cout << getExecRoot() << endl;
    cout << getExecRoot() / "py" << endl;

    path p = getExecRoot() / "py";
    for (directory_entry& x : directory_iterator(p)) {
        cout << '\t' << x.path() << '\n';
    }
}

BOOST_AUTO_TEST_SUITE_END()
