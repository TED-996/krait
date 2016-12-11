#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Krait
#include <boost/test/unit_test.hpp>
#include<iostream>


// #include"path_tests.cpp"
// #include"python_tests.cpp"

#include"python_worker.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(mod_Python)

BOOST_AUTO_TEST_CASE(funcs_python){
    cout << "\nTesting python:\n";

    initPython("/home/ted/proiect/tests/server");

    pythonRun("abc = [1, '2', 3, ('a', 'b'), {'c':'d'}]");

    cout << pythonEval("abc") << '\n';
}

BOOST_AUTO_TEST_SUITE_END()