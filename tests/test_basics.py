
import simplecoremidi as scm

def test_can_setup_midi():
    scm.setup_midi_output()


def test_can_send_midi():
    midion = (0x91, 0x3C, 0x7f)
    scm.setup_midi_output()
    scm.send_midi(midion)

