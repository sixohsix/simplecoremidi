#include <pthread.h>
#include <mach/mach_time.h>
#include <CoreMIDI/CoreMIDI.h>
#include <Python/Python.h>


typedef struct _SCMData {
  MIDIClientRef theMidiClient;
  MIDIEndpointRef midiSource;
  MIDIEndpointRef midiDestination;
  CFMutableDataRef receivedMidi;
} SCMData;

static SCMData _scmData;


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


static PyObject *
setupMidiOutput(PyObject* self, PyObject* args) {
  MIDIClientCreate(CFSTR("simple core midi client"), NULL, NULL,
                   &(_scmData.theMidiClient));
  MIDISourceCreate(_scmData.theMidiClient, CFSTR("simple core midi source"),
                   &(_scmData.midiSource));

  _scmData.receivedMidi = CFDataCreateMutable(kCFAllocatorDefault, 0);
  MIDIDestinationCreate(_scmData.theMidiClient,
                        CFSTR("simple core midi destination"),
                        recvMidiProc,
                        NULL,
                        &(_scmData.midiDestination));
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject*
sendMidi(PyObject* self, PyObject* args) {
  PyObject* midiData;
  Py_ssize_t nBytes;
  char pktListBuf[1024+100];
  MIDIPacketList* pktList = (MIDIPacketList*) pktListBuf;
  MIDIPacket* pkt;
  Byte midiDataToSend[1024];
  UInt64 now;
  int i;

  midiData = PyTuple_GetItem(args, 0);
  nBytes = PySequence_Size(midiData);

  for (i = 0; i < nBytes; i++) {
    PyObject* midiByte;

    midiByte = PySequence_GetItem(midiData, i);
    midiDataToSend[i] = PyInt_AsLong(midiByte);
  }

  now = mach_absolute_time();
  pkt = MIDIPacketListInit(pktList);
  pkt = MIDIPacketListAdd(pktList, 1024+100, pkt, now, nBytes, midiDataToSend);

  if (pkt == NULL || MIDIReceived(_scmData.midiSource, pktList)) {
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
  {"send_midi", sendMidi, METH_VARARGS, "Send midi data tuple."},
  {"recv_midi", recvMidi, METH_VARARGS, "Receive midi data tuple."},
  {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC
init_simplecoremidi(void) {
  (void) Py_InitModule("_simplecoremidi", SimpleCoreMidiMethods);
}
