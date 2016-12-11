#include<python2.7/Python.h>
#include<boost/python.hpp>
#include<string>
#include"python_worker.h"
#include"path.h"

using namespace boost::python;
using namespace boost::filesystem;

static object mainGlobal;

void initPython(std::string projectDir){
    Py_Initialize();

    object main = import("__main__");
    mainGlobal = main.attr("__dict__");

    mainGlobal.attr("project_dir") = projectDir;
    
}
