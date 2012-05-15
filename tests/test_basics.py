
import simplecoremidi as scm

midion = (0x91, 0x3C, 0x7f)
midioff = (0x91, 0x3C, 0x00)


def test_can_send_midi():
    scm.send_midi(midion)
    scm.send_midi(midioff)


def test_can_send_midi_as_list():
    midion = [0x91, 0x3C, 0x7f]
    midioff = [0x91, 0x3C, 0x00]
    scm.send_midi(midion)
    scm.send_midi(midioff)


def test_can_create_source_and_send():
    source = scm.MIDISource()
    source.send(midion)
    source.send(midioff)


def test_can_create_dest_and_recv():
    dest = scm.MIDIDestination()
    assert () == dest.recv()
