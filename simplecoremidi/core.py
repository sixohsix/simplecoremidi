from . import _simplecoremidi as cfuncs

def setup_midi_output():
    cfuncs.setup_midi_output()


def send_midi(midi_data):
    assert isinstance(midi_data, tuple)
    return cfuncs.send_midi(midi_data)

