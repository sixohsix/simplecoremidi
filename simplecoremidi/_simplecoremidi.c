#include <pthread.h>
#include <mach/mach_time.h>
#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Python/Python.h>


typedef struct _SCMData {
  MIDIEndpointRef midiDestination;
  CFMutableDataRef receivedMidi;
} SCMData;

static SCMData _scmData;

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
recvMidiProc(const MIDIPacketList* pktList,
             void* readProcRefCon,
             void* srcConnRefCon) {
  int i;
  const MIDIPacket* pkt;

  pkt = &pktList->packet[0];
  for (i = 0; i < pktList->numPackets; i++) {
    CFDataAppendBytes(_scmData.receivedMidi, pkt->data, pkt->length);
    pkt = MIDIPacketNext(pkt);
  }
}


static void
SCMMIDIEndpointDispose(void* ptr) {
  MIDIEndpointRef endpoint = (MIDIEndpointRef) ptr;
  MIDIEndpointDispose(endpoint);
}


static PyObject *
SCMCreateMIDISource(PyObject* self, PyObject* args) {
  MIDIClientRef midiClient;
  MIDIEndpointRef midiSource;
  char* midiSourceName;
  CFStringRef midiSourceNameCFS;

  midiSourceName = PyString_AsString(PyTuple_GetItem(args, 0));
  midiSourceNameCFS = CFStringCreateWithCString(NULL,
                                                midiSourceName,
                                                kCFStringEncodingUTF8);
  midiClient = SCMGlobalMIDIClient();
  MIDISourceCreate(midiClient, midiSourceNameCFS, &midiSource);
  CFRelease(midiSourceNameCFS);

  return PyCObject_FromVoidPtr(midiSource, SCMMIDIEndpointDispose);
}

static PyObject *
setupMidiOutput(PyObject* self, PyObject* args) {
  MIDIClientRef midiClient = SCMGlobalMIDIClient();

  _scmData.receivedMidi = CFDataCreateMutable(kCFAllocatorDefault, 0);
  MIDIDestinationCreate(midiClient,
                        CFSTR("simple core midi destination"),
                        recvMidiProc,
                        NULL,
                        &(_scmData.midiDestination));
  Py_INCREF(Py_None);
  return Py_None;
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
recvMidi(PyObject* self, PyObject* args) {
  PyObject* receivedMidiT;
  int i;
  Byte buf[1024];
  CFRange byteRange;
  CFIndex numBytes;

  numBytes = CFDataGetLength(_scmData.receivedMidi);
  if (numBytes > 1024) {
    numBytes = 1024;
  }

  byteRange = CFRangeMake(0, numBytes);
  CFDataGetBytes(_scmData.receivedMidi, byteRange, buf);
  CFDataDeleteBytes(_scmData.receivedMidi, byteRange);

  receivedMidiT = PyTuple_New(numBytes);
  for (i = 0; i < numBytes; i++) {
    PyObject* midiByte = PyInt_FromLong(buf[i]);
    PyTuple_SetItem(receivedMidiT, i, midiByte);
  }

  return receivedMidiT;
}


static PyMethodDef SimpleCoreMidiMethods[] = {
  {"setup_midi_output", setupMidiOutput, METH_VARARGS, "Setup midi."},
  {"send_midi", SCMSendMidi, METH_VARARGS, "Send midi data tuple via source."},
  {"recv_midi", recvMidi, METH_VARARGS, "Receive midi data tuple."},
  {"create_source", SCMCreateMIDISource, METH_VARARGS, "Create a new MIDI source."},
  {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC
init_simplecoremidi(void) {
  (void) Py_InitModule("_simplecoremidi", SimpleCoreMidiMethods);
}
