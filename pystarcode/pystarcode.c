#include <Python.h>
#include <stdlib.h>
#include "../src/starcode.h"
#include "../src/trie.h"

#define MAX_STR_LENGTH 2048

// module docstrings
static char module_docstring[] = 
       "This module is a Python interface to Starcode";
static char starcode_c_docstring[] = 
       "Starcode invocation";
static char starcode_docstring[] = 
       "Starcode invocation";

// the module methods declaration
static PyObject* pystarcode_starcode_c(PyObject *self, PyObject *args, PyObject *kwargs);

static PyObject* pystarcode_starcode(PyObject *self, PyObject *args, PyObject *kwargs);

static PyMethodDef module_methods[] = {
  {"starcode_c", (PyCFunction) pystarcode_starcode_c,
    METH_VARARGS|METH_KEYWORDS, starcode_c_docstring},
  {"starcode", (PyCFunction) pystarcode_starcode,
    METH_VARARGS|METH_KEYWORDS, starcode_docstring},
  {NULL, NULL, 0, NULL}
};

// init module method
PyMODINIT_FUNC initpystarcode(void)
{
  PyObject *m = Py_InitModule3("pystarcode", module_methods, module_docstring);
  if (m == NULL)
    return;
}

// a python-friendly file opener
FILE *py_fopen(const char *fname, const char *mode)
{
  FILE *f = fopen(fname, mode);
  if (f==NULL)
  {
    char error_message[MAX_STR_LENGTH];
    sprintf(error_message, "Cannot open file %s", fname);
    PyErr_SetString(PyExc_IOError, error_message);
    return NULL;
  }
  else
    return f;
}

// this method is used to invoke exactly starcode from within python, without
// any system call
static PyObject* pystarcode_starcode_c(PyObject *self, PyObject *args, PyObject *kwargs)
{
  // Parse the input
  char *in_filename, *out_filename;
  int tau, cluster_ratio;
  int clusteralg = 0;
  int verbose = 1;
  int thrmax = 4;
  int showclusters = 1;
  int showids = 0;
  int outputt = 0;

  static char *kwlist[] = {
    "input",
    "output",
    "dist",
    "cluster_ratio",
    "clusteralg",
    "verbose",
    "threads",
    "showclusters",
    "showids",
    "outputt",
    NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ssii|iiiiii", kwlist,
	&in_filename,
	&out_filename,
	&tau,
	&cluster_ratio,
	&clusteralg,
	&verbose,
	&thrmax,
	&showclusters,
	&showids,
	&outputt))
    return NULL;

  // open input and output files
  FILE *inputf1 = py_fopen(in_filename,"r");
  if (inputf1 == NULL) return NULL;
  FILE *inputf2 = NULL;
  FILE *outputf1 = py_fopen(out_filename, "w");
  if (outputf1 == NULL) return NULL;
  FILE *outputf2 = NULL;

  printf("%s %s %d %d %d %d\n",in_filename,out_filename,tau,cluster_ratio,clusteralg,verbose);

  // init the "gstack_t" data structure, which will contain the information on
  // the sequences to analyze
  gstack_t *uSQ = read_file(inputf1, inputf2, verbose);

  int out = starcode(
      uSQ,
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

  // int out = 0;

  PyObject *ret = Py_BuildValue("h", out);

  return ret;
}

static PyObject* pystarcode_starcode(PyObject *self, PyObject *args, PyObject *kwargs)
{
  // Parse the input
  int tau, cluster_ratio;
  int clusteralg = 0;
  int verbose = 1;
  int thrmax = 4;
  int showclusters = 1;
  int showids = 0;
  int outputt = 0;
  PyObject * in_list;

  static char *kwlist[] = {
    "input_list",
    "dist",
    "cluster_ratio",
    "clusteralg",
    "verbose",
    "threads",
    "showclusters",
    "showids",
    "outputt",
    NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ii|iiiiii", kwlist,
	&PyList_Type,
	&in_list,
	&tau,
	&cluster_ratio,
	&clusteralg,
	&verbose,
	&thrmax,
	&showclusters,
	&showids,
	&outputt))
    return NULL;

  // get the number of sequence elements in the input list
  int numLines = PyList_Size(in_list);

  // init the return object
  PyObject * retList = PyList_New(numLines); /* the list to return */

  // should raise an error here
  if (numLines < 0) {
    PyErr_SetString(PyExc_ValueError, "Input element is not list.");
    return NULL;
  }

  // init the structure that we will pass to the starcode core function
  gstack_t *uSQ = new_gstack();

  // build the list of sequences to pass to the starcode core function
  for (size_t i=0; i<numLines; i++){
    PyObject * strObj;

    // grab the string object from the next element of the list */
    strObj = PyList_GetItem(in_list, i);

    // check if passed element is string
    if (!PyObject_TypeCheck(strObj, &PyBaseString_Type)) {
      PyErr_SetString(PyExc_ValueError, "Input element is not string.");
      return NULL;
    }

    // convert to a C string
    char *sequence = PyString_AsString(strObj);
    size_t seqlen = PyString_Size(strObj);

    // check that the sequence is sane
    for (size_t j = 0; j<seqlen; j++) {
      if (!valid_DNA_char[(int)sequence[j]]) {
	PyErr_SetString(PyExc_ValueError, "Invalid sequence encountered.");
	return NULL;
      }
    }

    // init new sequence and put it in the stack
    // TODO: check that this NULL argument makes sense
    useq_t *new = new_useq(1, sequence, NULL);
    new->nids = 1;
    new->seqid = (void *)(unsigned long)uSQ->nitems+1;
    push(new, &uSQ);

    if (PyList_SetItem(retList, i, strObj) == -1)
      Py_RETURN_FALSE;
  }

  return retList;
}
