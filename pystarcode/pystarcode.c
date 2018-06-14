#include <Python.h>
#include <stdlib.h>
#include "../src/starcode.h"
#include "../src/trie.h"

#define MAX_STR_LENGTH 2048

// module docstrings
static char module_docstring[] = 
       "This module is a Python interface to Starcode";
static char starcode_docstring[] = 
       "Starcode list of input sequences";

// the module methods declaration
static PyObject* pystarcode_starcode(PyObject *self, PyObject *args, PyObject *kwargs);

static PyMethodDef module_methods[] = {
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
  }

  // we're now ready to invoke the main starcode core function
  gstack_t *clusters = starcode(
      uSQ,
      tau,
      verbose,
      thrmax,
      clusteralg,
      cluster_ratio,
      showids
  );

  // init the return object: a python dictionary for the sequence -> canonical
  // association, and another dictionary for the canonical -> counts association
  PyObject * d = PyDict_New();
  PyObject * counts = PyDict_New();

  // init the canonical sequence
  useq_t *canonical = ((useq_t *)clusters->items[0])->canonical;
  PyDict_SetItemString(counts, canonical->seq, PyInt_FromSize_t(canonical->count));

  // fill in the dictionaries
  for (size_t i = 0 ; i < clusters->nitems ; i++) {
    useq_t *u = (useq_t *) clusters->items[i];
    PyDict_SetItemString(d, u->seq, PyString_FromString(u->canonical->seq));

    // let's see if the current canonical is the same as the last one
    if (u->canonical != canonical) {

      // in this case we update the canonical
      canonical = u->canonical;

      // and we write the new element in the dictionary of counts
      PyDict_SetItemString(counts, canonical->seq, PyInt_FromSize_t(canonical->count));
    }
  }

  // let's now build the return object, which will be a tuple of the two
  // dictionaries
  PyObject *ret = PyTuple_New(2);
  PyTuple_SetItem(ret, 0, counts);
  PyTuple_SetItem(ret, 1, d);

  // return the created tuple
  return ret;
}
