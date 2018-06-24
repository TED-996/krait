#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Krait
#include <boost/test/unit_test.hpp>
#include <iostream>


#include "except.h"
#include "pythonModule.h"

BOOST_AUTO_TEST_SUITE(mod_Python)

BOOST_AUTO_TEST_CASE(funcs_python) {
    try {
        std::cout << "\nTesting python:" << std::endl;

        PythonModule::initModules("/home/ted/proiect/tests/server");
        std::cout << "Python inited." << std::endl;

        PythonModule::main().run("abc = [1, '2', 3, ('a', 'b'), {'c':'d'}]");

        std::cout << "Run called" << std::endl;

        std::cout << PythonModule::main().eval("abc") << '\n';
        std::cout << PythonModule::main().eval("True") << ' ' << PythonModule::main().test("'abc'") << ' '
                  << PythonModule::main().test("0") << ' ' << PythonModule::main().test("False") << std::endl;
    } catch (pythonError& err) {
        std::cout << "caught error!" << std::endl;
        if (const std::string* errS = boost::get_error_info<stringInfo>(err)) {
            std::cerr << errS;
        }
    } catch (std::exception& ex) {
        std::cout << ex.what() << '\n';
    }
}

BOOST_AUTO_TEST_SUITE_END()
