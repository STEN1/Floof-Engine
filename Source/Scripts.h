#ifndef FLOOF_SCRIPTS_H
#define FLOOF_SCRIPTS_H

//pulls in python api
#define PY_SSIZE_T_CLEAN
#ifdef _DEBUG
#undef _DEBUG
#include <python.h>
#define _DEBUG
#else
#include <Python.h>
#endif




#endif //FLOOF_SCRIPTS_H
