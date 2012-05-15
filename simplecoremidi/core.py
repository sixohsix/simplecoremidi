from . import _simplecoremidi as cfuncs

_midi_set_up = False

def _maybe_setup_midi():
    global _midi_set_up
    if not _midi_set_up:
        cfuncs.setup_midi_output()
        _midi_set_up = True

_global_midi_source = None
def _get_global_midi_source():
    global _global_midi_source
    if _global_midi_source is None:
        _global_midi_source = MIDISource("simple core midi source")
    return _global_midi_source


class MIDISource(object):
    def __init__(self, source_name=None):
        if not source_name:
            source_name = "unnamed source"
        self._source = cfuncs.create_source(source_name)

    def send(self, midi_data):
        assert isinstance(midi_data, tuple) or isinstance(midi_data, list)
        print midi_data
        return cfuncs.send_midi(self._source, midi_data)


def send_midi(midi_data):
    return _get_global_midi_source().send(midi_data)


def recv_midi():
    _maybe_setup_midi()
    return cfuncs.recv_midi()
