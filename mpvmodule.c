/* Copyright (c) 2014 Ariën Holthuizen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <Python.h>
#include "client.h"

typedef struct {
  PyObject_HEAD
  mpv_handle *handle;
} MPV;

static void MPV_dealloc(MPV* self) {
  mpv_destroy(self->handle);
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject *MPV_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  MPV *self;
  self = (MPV *)type->tp_alloc(type, 0);
  self->handle = mpv_create();
  return (PyObject *)self;
}

static int MPV_init(MPV *self, PyObject *args, PyObject *kwds) {
  PyObject *options=NULL;

  static char *kwlist[] = {"options"};

  if(! PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist,
                                   &options))
    return -1;

  if (options) {
    PyObject *iterator = PyObject_GetIter(PyMapping_Items(options));
    PyObject *item;
    const char *name, *value;
    int error;
    if (iterator == NULL)
      return -1;
    while (item = PyIter_Next(iterator)){
      if (!PyArg_ParseTuple(item, "ss", &name, &value)) 
        return -1;
      Py_DECREF(item);
      if(error = mpv_set_option_string(self->handle, name, value) < 0) {
        PyErr_SetString(PyExc_RuntimeError, mpv_error_string(error));
        return -1;
      }
    }
    Py_DECREF(iterator);
  }
  mpv_initialize(self->handle);
}

static PyObject *MPV_command(MPV *self, PyObject *args) {
  const char *command;
  int error;
  if (!PyArg_ParseTuple(args, "s", &command))
    return NULL;
  if(error = mpv_command_string(self->handle, command) < 0) {
    PyErr_SetString(PyExc_RuntimeError, mpv_error_string(error));
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *MPV_set_property(MPV *self, PyObject *args) {
  const char *name, *data;
  int error;
  if (!PyArg_ParseTuple(args, "ss", &name, &data))
    return NULL;
  if(error = mpv_set_property_string(self->handle, name, data) < 0) {
    PyErr_SetString(PyExc_RuntimeError, mpv_error_string(error));
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *MPV_get_property(MPV *self, PyObject *args) {
  const char *name;
  char *value;
  int error;
  PyObject *retval;
  if (!PyArg_ParseTuple(args, "s", &name))
    return NULL;
  error = mpv_get_property(self->handle, name, MPV_FORMAT_STRING, &value);
  if (error < 0) {
    PyErr_SetString(PyExc_RuntimeError, mpv_error_string(error));
    return NULL;
  }
  retval = PyString_FromString(value);
  mpv_free(value);
  return retval;
}

static PyObject *MPV_wait_event(MPV *self, PyObject *args) {
  double timeout;
  const char *eventname;
  mpv_event *event;
  if (!PyArg_ParseTuple(args, "d", &timeout))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  event = mpv_wait_event(self->handle, timeout);
  Py_END_ALLOW_THREADS
  eventname = mpv_event_name(event->event_id);
  return Py_BuildValue("ss", eventname, event->data);
}

static PyMethodDef MPV_methods[] = {
  {"command", (PyCFunction)MPV_command, METH_VARARGS,
   "Send a command to an initialized player"
  },
  {"set_property", (PyCFunction)MPV_set_property, METH_VARARGS,
   "Set a property on an initialized player"
  },
  {"get_property", (PyCFunction)MPV_get_property, METH_VARARGS,
   "Get a property from an initialized player"
  },
  {"wait_event", (PyCFunction)MPV_wait_event, METH_VARARGS,
   "Block until an event happens"
  },
  {NULL}
};

static PyTypeObject MPVType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "MPV.mpv",                 /*tp_name*/
  sizeof(MPV),               /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)MPV_dealloc,   /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "MPV objects",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  MPV_methods,             /* tp_methods */
  0,             /* tp_members */
  0,           /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)MPV_init,      /* tp_init */
  0,                         /* tp_alloc */
  MPV_new,                 /* tp_new */
};

static PyMethodDef module_methods[] = {
  {NULL}
};

#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initmpv(void) {
  PyObject* m;
  if (PyType_Ready(&MPVType) < 0)
    return;
  m = Py_InitModule3("mpv", MPV_methods,
                     "Module to talk to MPV.");

  Py_INCREF(&MPVType);
  PyModule_AddObject(m, "MPV", (PyObject *)&MPVType);
}
