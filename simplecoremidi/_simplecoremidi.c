#include <CoreMIDI/CoreMIDI.h>
#include <Python/Python.h>


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
  int i;

  midiData = PyTuple_GetItem(args, 0);
  nBytes = PyTuple_Size(midiData);

  midiDataToSend = (Byte*) malloc(nBytes);

  for (i = 0; i < nBytes; i++) {
    PyObject* midiByte;

    midiByte = PyTuple_GetItem(midiData, i);
    midiDataToSend[i] = PyInt_AsLong(midiByte);
    printf("%d, ", midiDataToSend[i]);
  }

  pktList = (MIDIPacketList*) malloc(1024);
  pkt = MIDIPacketListInit(pktList);
  pkt = MIDIPacketListAdd(pktList, 1024, pkt, 0, nBytes, midiDataToSend);

  if (pkt == NULL || MIDISend(_cmData.outPort, _cmData.midiOut, pktList)) {
    printf("failed to send the midi.\n");
  } else {
    printf("sent!\n");
  }

  free(pktList);
  free(midiDataToSend);

  //CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, false);

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


int main(int argc, char* args[]) {
  MIDIClientRef theMidiClient;
  MIDIEndpointRef midiOut;
  MIDIPortRef outPort;
  char pktBuffer[1024];
  MIDIPacketList* pktList = (MIDIPacketList*) pktBuffer;
  MIDIPacket* pkt;
  Byte midiDataToSend[] = {0x91, 0x3c, 0x40};
  int i;

  MIDIClientCreate(CFSTR("Magical MIDI"), NULL, NULL,
                   &(theMidiClient));
  MIDISourceCreate(theMidiClient, CFSTR("Magical MIDI Source"),
                   &(midiOut));
  MIDIOutputPortCreate(theMidiClient, CFSTR("Magical MIDI Out Port"),
                       &(outPort));

  pkt = MIDIPacketListInit(pktList);
  pkt = MIDIPacketListAdd(pktList, 1024, pkt, 0, 3, midiDataToSend);

  for (i = 0; i < 100; i++) {
    if (pkt == NULL || MIDISend(outPort, midiOut, pktList)) {
      printf("failed to send the midi.\n");
    } else {
      printf("sent!\n");
    }
    sleep(1);
  }

  return 0;
}
