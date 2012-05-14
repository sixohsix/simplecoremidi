from . import _simplecoremidi as cfuncs

_midi_set_up = False

def _maybe_setup_midi():
    global _midi_set_up
    if not _midi_set_up:
        cfuncs.setup_midi_output()
        _midi_set_up = True


def send_midi(midi_data):
    """
    Send MIDI data out via the simple core midi source.
    """
    _maybe_setup_midi()
    assert isinstance(midi_data, tuple) or isinstance(midi_data, list)
    return cfuncs.send_midi(midi_data)


def recv_midi():
    _maybe_setup_midi()
    return cfuncs.recv_midi()
