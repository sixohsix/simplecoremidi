
import simplecoremidi as scm

def test_can_send_midi():
    midion = (0x91, 0x3C, 0x7f)
    midioff = (0x91, 0x3C, 0x00)
    scm.send_midi(midion)
    scm.send_midi(midioff)


def test_can_send_midi_as_list():
    midion = [0x91, 0x3C, 0x7f]
    midioff = [0x91, 0x3C, 0x00]
    scm.send_midi(midion)
    scm.send_midi(midioff)
