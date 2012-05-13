from . import _simplecoremidi as cfuncs

_midi_set_up = False

def send_midi(midi_data):
    """
    Send MIDI data out via the simple core midi source.
    """
    global _midi_set_up
    assert isinstance(midi_data, tuple)
    if not _midi_set_up:
        cfuncs.setup_midi_output()
        _midi_set_up = True
    return cfuncs.send_midi(midi_data)
