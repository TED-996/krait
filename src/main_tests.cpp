#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Krait
#include <boost/test/unit_test.hpp>
#include<iostream>


#include"except.h"
#include "pythonModule.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(mod_Python)

BOOST_AUTO_TEST_CASE(funcs_python) {
	try {
		cout << "\nTesting python:" << endl;

		PythonModule::initModules("/home/ted/proiect/tests/server");
		cout << "Python inited." << endl;

		PythonModule::main.run("abc = [1, '2', 3, ('a', 'b'), {'c':'d'}]");

		cout << "Run called" << endl;

		cout << PythonModule::main.eval("abc") << '\n';
		cout << PythonModule::main.eval("True") << ' ' << PythonModule::main.test("'abc'") << ' '
		     << PythonModule::main.test("0") << ' ' << PythonModule::main.test("False") << endl;
	}
	catch (pythonError& err) {
		cout << "caught error!" << endl;
		if (const string* errS = boost::get_error_info<stringInfo>(err)) {
			cerr << errS;
		}
	}
	catch (exception& ex) {
		cout << ex.what() << '\n';
	}
}

BOOST_AUTO_TEST_SUITE_END()
