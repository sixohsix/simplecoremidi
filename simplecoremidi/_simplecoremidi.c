#include <pthread.h>
#include <mach/mach_time.h>
#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Python/Python.h>


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


static void
SCMMIDIEndpointDispose(void* ptr) {
  MIDIEndpointRef endpoint = (MIDIEndpointRef) ptr;
  MIDIEndpointDispose(endpoint);
}


static PyObject *
SCMCreateMIDISource(PyObject* self, PyObject* args) {
  MIDIEndpointRef midiSource;
  CFStringRef midiSourceName;

  midiSourceName =
    CFStringCreateWithCString(NULL,
                              PyString_AsString(PyTuple_GetItem(args, 0)),
                              kCFStringEncodingUTF8);
  MIDISourceCreate(SCMGlobalMIDIClient(), midiSourceName, &midiSource);
  CFRelease(midiSourceName);

  return PyCObject_FromVoidPtr(midiSource, SCMMIDIEndpointDispose);
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

  midiSource = (MIDIEndpointRef*) PyCObject_AsVoidPtr(PyTuple_GetItem(args, 0));
  midiData = PyTuple_GetItem(args, 1);
  nBytes = PySequence_Size(midiData);

  for (i = 0; i < nBytes; i++) {
    PyObject* midiByte;

    midiByte = PySequence_GetItem(midiData, i);
    midiDataToSend[i] = PyInt_AsLong(midiByte);
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
                              PyString_AsString(PyTuple_GetItem(args, 0)),
                              kCFStringEncodingUTF8);
  destRef = SCMMIDIDestinationCreate(midiDestinationName);
  CFRelease(midiDestinationName);
  return PyCObject_FromVoidPtr(destRef, SCMMIDIDestinationDispose);
}


static PyObject*
SCMRecvMidi(PyObject* self, PyObject* args) {
  PyObject* receivedMidiT;
  UInt8* bytePtr;
  int i;
  CFIndex numBytes;
  SCMMIDIDestinationRef destRef
    = (SCMMIDIDestinationRef) PyCObject_AsVoidPtr(PyTuple_GetItem(args, 0));

  numBytes = CFDataGetLength(destRef->receivedMidi);

  receivedMidiT = PyTuple_New(numBytes);
  bytePtr = CFDataGetMutableBytePtr(destRef->receivedMidi);
  for (i = 0; i < numBytes; i++, bytePtr++) {
    PyObject* midiByte = PyInt_FromLong(*bytePtr);
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


PyMODINIT_FUNC
init_simplecoremidi(void) {
  (void) Py_InitModule("_simplecoremidi", SimpleCoreMidiMethods);
}
