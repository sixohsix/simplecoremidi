
#include <CoreMIDI/CoreMIDI.h>
#include <Python/Python.h>


int main(int argc, char* argv[]) {
  Byte midiData[] = {0x90, 0x3c, 0x80};
  MIDIClientRef theMidiClient;
  MIDIEndpointRef midiOut;
  MIDIPortRef outPort;
  MIDIPacketList *pktList;
  MIDIPacket *pkt;

  MIDIClientCreate(CFSTR("Magical MIDI"), NULL, NULL, &theMidiClient);
  MIDISourceCreate(theMidiClient, CFSTR("Magical MIDI Source"), &midiOut);
  MIDIOutputPortCreate(theMidiClient, CFSTR("Magical MIDI Out Port"), &outPort);

  pktList = (MIDIPacketList*) malloc(1024);
  pkt = MIDIPacketListInit(pktList);

  pkt = MIDIPacketListAdd(pktList, 1024, pkt, 0, 3, midiData);
  MIDISend(outPort, midiOut, pktList);

  return 0;
}


typedef struct _CMData {
  MIDIClientRef theMidiClient;
  MIDIEndpointRef midiOut;
  MIDIPortRef outPort;
} CMData;

static CMData _cmData;


static PyObject *
setupMidiOutput(PyObject* self, PyObject* args) {
  MIDIClientCreate(CFSTR("Magical MIDI"), NULL, NULL,
                   &(_cmData.theMidiClient));
  MIDISourceCreate(_cmData.theMidiClient, CFSTR("Magical MIDI Source"),
                   &(_cmData.midiOut));
  MIDIOutputPortCreate(_cmData.theMidiClient, CFSTR("Magical MIDI Out Port"),
                       &(_cmData.outPort));
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
sendMidi(PyObject* self, PyObject* args) {
  PyObject* midiData;
  Py_ssize_t nBytes;
  MIDIPacketList *pktList;
  MIDIPacket *pkt;
  Byte* midiDataToSend;

  midiData = PyTuple_GetItem(args, 0);
  nBytes = PyTuple_Size(midiData);

  printf("n data bytes: %ld", nBytes);

  midiDataToSend = (Byte*) malloc(nBytes);

  for (int i = 0; i < nBytes; i++) {
    PyObject* midiByte;

    midiByte = PyTuple_GetItem(midiData, i);
    midiDataToSend[i] = PyInt_AsLong(midiByte);
    printf("data byte: %d", midiDataToSend[i]);
  }

  pktList = (MIDIPacketList*) malloc(1024);
  pkt = MIDIPacketListInit(pktList);

  pkt = MIDIPacketListAdd(pktList, 1024, pkt, 0, nBytes, midiDataToSend);

  if (MIDISend(_cmData.outPort, _cmData.midiOut, pktList)) {
    printf("Failed to send the midi.");
  }

  free(pktList);
  free(midiDataToSend);

  Py_INCREF(Py_None);
  return Py_None;
}


static PyMethodDef SimpleCoreMidiMethods[] = {
  {"setup_midi_output", setupMidiOutput, METH_VARARGS, "Setup midi."},
  {"send_midi", sendMidi, METH_VARARGS, "Send midi data tuple."},
  {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC
initsimplecoremidi(void) {
  (void) Py_InitModule("simplecoremidi", SimpleCoreMidiMethods);
}
