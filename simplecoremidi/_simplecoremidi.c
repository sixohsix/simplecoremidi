#include <CoreMIDI/CoreMIDI.h>
#include <Python/Python.h>


typedef struct _CMData {
  MIDIClientRef theMidiClient;
  MIDIEndpointRef midiSource;
} CMData;

static CMData _cmData;


static PyObject *
setupMidiOutput(PyObject* self, PyObject* args) {
  MIDIClientCreate(CFSTR("simple core midi client"), NULL, NULL,
                   &(_cmData.theMidiClient));
  MIDISourceCreate(_cmData.theMidiClient, CFSTR("simple core midi source"),
                   &(_cmData.midiSource));

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
sendMidi(PyObject* self, PyObject* args) {
  PyObject* midiData;
  Py_ssize_t nBytes;
  char pktListBuf[1024+100];
  MIDIPacketList* pktList = (MIDIPacketList*) pktListBuf;
  MIDIPacket* pkt;
  Byte midiDataToSend[1024];
  int i;

  midiData = PyTuple_GetItem(args, 0);
  nBytes = PyTuple_Size(midiData);

  for (i = 0; i < nBytes; i++) {
    PyObject* midiByte;

    midiByte = PyTuple_GetItem(midiData, i);
    midiDataToSend[i] = PyInt_AsLong(midiByte);
  }

  pkt = MIDIPacketListInit(pktList);
  pkt = MIDIPacketListAdd(pktList, 1024+100, pkt, 0, nBytes, midiDataToSend);

  if (pkt == NULL || MIDIReceived(_cmData.midiSource, pktList)) {
    printf("failed to send the midi.\n");
  }

  Py_INCREF(Py_None);
  return Py_None;
}


static PyMethodDef SimpleCoreMidiMethods[] = {
  {"setup_midi_output", setupMidiOutput, METH_VARARGS, "Setup midi."},
  {"send_midi", sendMidi, METH_VARARGS, "Send midi data tuple."},
  {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC
init_simplecoremidi(void) {
  (void) Py_InitModule("_simplecoremidi", SimpleCoreMidiMethods);
}
