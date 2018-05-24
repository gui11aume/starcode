#include <Python.h>
#include <stdlib.h>
#include "../src/starcode.h"

#define MAX_STR_LENGTH 2048

static char module_docstring[] = 
       "This module is a Python interface to Starcode";
static char starcode_docstring[] = 
       "Starcode invocation";

static PyObject *pystarcode_starcode(PyObject *self, PyObject *args);

static PyMethodDef module_methods[] = {
  {"starcode", pystarcode_starcode, METH_VARARGS, starcode_docstring},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpystarcode(void)
{
  PyObject *m = Py_InitModule3("pystarcode", module_methods, module_docstring);
  if (m == NULL)
    return;
}

static PyObject *pystarcode_starcode(PyObject *self, PyObject *args)
{
  // Parse the input
  char *in_filename, *out_filename;
  int tau, cluster_ratio;
  if (!PyArg_ParseTuple(args, "sshh",
	&in_filename,
	&out_filename,
	&tau,
	&cluster_ratio))
    return NULL;

  FILE *inputf1 = fopen(in_filename,"r");
  if (inputf1==NULL)
  {
    char error_message[MAX_STR_LENGTH];
    sprintf(error_message, "Cannot open %s for reading", in_filename);
    PyErr_SetString(PyExc_IOError, error_message);
    return NULL;
  }
  FILE *inputf2 = NULL;
  FILE *outputf1 = fopen(out_filename, "w");
  FILE *outputf2 = NULL;
  const int verbose = 1;
  int thrmax = 4;
  const int clusteralg = 0;
  const int showclusters = 1;
  const int showids = 0;
  const int outputt = 0;

  int out = starcode(
      inputf1,
      inputf2,
      outputf1,
      outputf2,
      tau,
      verbose,
      thrmax,
      clusteralg,
      cluster_ratio,
      showclusters,
      showids,
      outputt
  );


  PyObject *ret = Py_BuildValue("h", out);

  return ret;
}
