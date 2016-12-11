#include<python2.7/Python.h>
#include<boost/python.hpp>
#include"python_worker.h"


void initPython(){
    Py_Initialize();
}
