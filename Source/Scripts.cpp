#ifndef FLOOF_SCRIPTS_CPP
#define FLOOF_SCRIPTS_CPP

//pulls in python api NB ALLWAYS TOP OF INCLUDES
#define PY_SSIZE_T_CLEAN
#ifdef WIN32
#ifdef _DEBUG
        #undef _DEBUG
        #include <python.h>
        #define _DEBUG
    #else
        #include <Python.h>
    #endif
#else
#include <python.h>
#endif

#include <iostream>



int cfib(int n){
    if (n<2)
        return n;
    else
        return cfib(n-1)+cfib(n-2);

}

static PyObject * fib(PyObject* self, PyObject* args){

    int n = 1;
    if(!PyArg_ParseTuple(args, "i", &n))
       return NULL;

    return Py_BuildValue("i",cfib(n));

}

static PyObject * log(PyObject* self, PyObject *args){
    std::string message = "Calling cout from python";

    std::cout << message << std::endl;
    return Py_BuildValue("s", message.c_str());
}

static PyObject * Start(PyObject* self, PyObject*args){

    return NULL;
}

static PyObject * OnUpdate(PyObject* self, PyObject*args){

    return NULL;
}


static PyMethodDef Methods[] = {
        {"start",(PyCFunction)Start,METH_VARARGS, "On Entity Start"},
        {"onUpdate",(PyCFunction)OnUpdate,METH_NOARGS, "On Entity update"},
        {"printTest",(PyCFunction)log,METH_NOARGS, "print with cout"},
        {"fib",(PyCFunction)fib,METH_VARARGS, "Calculate fibonacci using cpp"},
        {NULL, NULL, 0 , NULL} //last method allways NULL
};

static struct PyModuleDef Modules = {
        PyModuleDef_HEAD_INIT,
        "Floof",
       "Floof Module",
       -1,
       Methods
};

PyMODINIT_FUNC PyInit_Floof(void){
    std::cout << "Floof Python Api Initialized" << std::endl;
    return PyModule_Create(&Modules);
}


#endif //FLOOF_SCRIPTS_CPP
