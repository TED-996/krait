#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Krait
#include <boost/test/unit_test.hpp>
#include<iostream>


// #include"path_tests.cpp"
// #include"python_tests.cpp"

#include"python_worker.h"
#include"except.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(mod_Python)

BOOST_AUTO_TEST_CASE(funcs_python) {
    try {
        cout << "\nTesting python:" << endl;

        initPython("/home/ted/proiect/tests/server");
        cout << "Python inited." << endl;

        pythonRun(string("abc = [1, '2', 3, ('a', 'b'), {'c':'d'}]"));

        cout << "Run called" << endl;

        cout << pythonEval(string("abc")) << '\n';
    }
    catch(pythonError& err) {
        cout << "caught error!" << endl;
        if (const string* errS = boost::get_error_info<stringInfo>(err)) {
            cerr << errS;
        }
    }
    catch(exception &ex) {
        cout << ex.what() << '\n';
    }
}

BOOST_AUTO_TEST_SUITE_END()
