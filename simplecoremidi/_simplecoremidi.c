#include <pthread.h>
#include <mach/mach_time.h>
#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Python.h>

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif



struct _SCMMIDIDestination {
  MIDIEndpointRef midiDestination;
  CFMutableDataRef receivedMidi;
};

typedef struct _SCMMIDIDestination* SCMMIDIDestinationRef;


static MIDIClientRef _midiClient;


static MIDIClientRef
SCMGlobalMIDIClient() {
  if (! _midiClient) {
    MIDIClientCreate(CFSTR("simple core midi client"), NULL, NULL,
                     &(_midiClient));
  }
  return _midiClient;
}

void
SCMRecvMIDIProc(const MIDIPacketList* pktList,
                void* readProcRefCon,
                void* srcConnRefCon) {
  SCMMIDIDestinationRef destRef = (SCMMIDIDestinationRef) readProcRefCon;
  int i;
  const MIDIPacket* pkt;

  pkt = &pktList->packet[0];
  for (i = 0; i < pktList->numPackets; i++) {
    CFDataAppendBytes(destRef->receivedMidi, pkt->data, pkt->length);
    pkt = MIDIPacketNext(pkt);
  }
}


SCMMIDIDestinationRef
SCMMIDIDestinationCreate(CFStringRef midiDestinationName) {
  SCMMIDIDestinationRef destRef
    = CFAllocatorAllocate(NULL, sizeof(struct _SCMMIDIDestination), 0);
  destRef->receivedMidi = CFDataCreateMutable(NULL, 0);
  MIDIDestinationCreate(SCMGlobalMIDIClient(),
                        midiDestinationName,
                        SCMRecvMIDIProc,
                        destRef,
                        &(destRef->midiDestination));
  return destRef;
}

void
SCMMIDIDestinationDispose(SCMMIDIDestinationRef destRef) {
  MIDIEndpointDispose(destRef->midiDestination);
  CFRelease(destRef->receivedMidi);
  CFAllocatorDeallocate(NULL, destRef);
}


static void MIDIEndpoint_Destructor(PyObject* obj) {
  MIDIEndpointRef midiEndpoint = (MIDIEndpointRef)PyCapsule_GetPointer(obj, NULL);
  MIDIEndpointDispose(midiEndpoint);
}


static void MIDIDest_Destructor(PyObject* obj) {
  SCMMIDIDestinationRef midiDest = (SCMMIDIDestinationRef)PyCapsule_GetPointer(obj, NULL);
  SCMMIDIDestinationDispose(midiDest);
}


static PyObject *
SCMCreateMIDISource(PyObject* self, PyObject* args) {
  MIDIEndpointRef midiSource;
  CFStringRef midiSourceName;

  midiSourceName =
    CFStringCreateWithCString(NULL,
#if PY_MAJOR_VERSION >= 3
                              PyUnicode_AsUTF8AndSize(PyTuple_GetItem(args, 0), NULL),
#else
			      PyString_AsString(PyTuple_GetItem(args, 0)),
#endif
                              kCFStringEncodingUTF8);
  MIDISourceCreate(SCMGlobalMIDIClient(), midiSourceName, &midiSource);
  CFRelease(midiSourceName);

  return PyCapsule_New((void*)midiSource, NULL, MIDIEndpoint_Destructor);
}

static PyObject*
SCMSendMidi(PyObject* self, PyObject* args) {
  MIDIEndpointRef midiSource;
  PyObject* midiData;
  Py_ssize_t nBytes;
  char pktListBuf[1024+100];
  MIDIPacketList* pktList = (MIDIPacketList*) pktListBuf;
  MIDIPacket* pkt;
  Byte midiDataToSend[1024];
  UInt64 now;
  int i;

  midiSource = (MIDIEndpointRef) PyCapsule_GetPointer(PyTuple_GetItem(args, 0), NULL);
  midiData = PyTuple_GetItem(args, 1);
  nBytes = PySequence_Size(midiData);

  for (i = 0; i < nBytes; i++) {
    PyObject* midiByte;

    midiByte = PySequence_GetItem(midiData, i);
#if PY_MAJOR_VERSION >= 3
    midiDataToSend[i] = PyLong_AsLong(midiByte);
#else
    midiDataToSend[i] = PyInt_AsLong(midiByte);
#endif
  }

  now = mach_absolute_time();
  pkt = MIDIPacketListInit(pktList);
  pkt = MIDIPacketListAdd(pktList, 1024+100, pkt, now, nBytes, midiDataToSend);

  if (pkt == NULL || MIDIReceived(midiSource, pktList)) {
    printf("failed to send the midi.\n");
  }

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject*
SCMCreateMIDIDestination(PyObject* self, PyObject* args) {
  SCMMIDIDestinationRef destRef;
  CFStringRef midiDestinationName;

  midiDestinationName =
    CFStringCreateWithCString(NULL,
#if PY_MAJOR_VERSION >= 3
                              PyUnicode_AsUTF8AndSize(PyTuple_GetItem(args, 0), NULL),
#else
			      PyString_AsString(PyTuple_GetItem(args, 0)),
#endif
                              kCFStringEncodingUTF8);
  destRef = SCMMIDIDestinationCreate(midiDestinationName);
  CFRelease(midiDestinationName);
  return PyCapsule_New(destRef, NULL, MIDIDest_Destructor);
}


static PyObject*
SCMRecvMidi(PyObject* self, PyObject* args) {
  PyObject* receivedMidiT;
  UInt8* bytePtr;
  int i;
  CFIndex numBytes;
  SCMMIDIDestinationRef destRef
    = (SCMMIDIDestinationRef) PyCapsule_GetPointer(PyTuple_GetItem(args, 0), NULL);

  numBytes = CFDataGetLength(destRef->receivedMidi);

  receivedMidiT = PyTuple_New(numBytes);
  bytePtr = CFDataGetMutableBytePtr(destRef->receivedMidi);
  for (i = 0; i < numBytes; i++, bytePtr++) {
#if PY_MAJOR_VERSION >= 3
    PyObject* midiByte = PyLong_FromLong(*bytePtr);
#else
    PyObject* midiByte = PyInt_FromLong(*bytePtr);
#endif
    PyTuple_SetItem(receivedMidiT, i, midiByte);
  }

  CFDataDeleteBytes(destRef->receivedMidi, CFRangeMake(0, numBytes));
  return receivedMidiT;
}


static PyMethodDef SimpleCoreMidiMethods[] = {
  {"send_midi", SCMSendMidi, METH_VARARGS, "Send midi data tuple via source."},
  {"recv_midi", SCMRecvMidi, METH_VARARGS, "Receive midi data tuple."},
  {"create_source", SCMCreateMIDISource, METH_VARARGS, "Create a new MIDI source."},
  {"create_destination", SCMCreateMIDIDestination, METH_VARARGS, "Create a new MIDI destination."},
  {NULL, NULL, 0, NULL}
};


#if PY_MAJOR_VERSION >= 3

static int simplecoremidi_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int simplecoremidi_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "simplecoremidi",
        NULL,
        sizeof(struct module_state),
        SimpleCoreMidiMethods,
        NULL,
        simplecoremidi_traverse,
        simplecoremidi_clear,
        NULL
};

#define INITERROR return NULL

PyObject *
PyInit__simplecoremidi(void)

#else
#define INITERROR return
void
init_simplecoremidi(void) 
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("_simplecoremidi", SimpleCoreMidiMethods);
#endif
    if (module == NULL)
      INITERROR;
    struct module_state *st = GETSTATE(module);
    
    st->error = PyErr_NewException("simplecoremidi.Error", NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }
#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
