from . import _simplecoremidi as cfuncs


class MIDISource(object):
    def __init__(self, source_name=None):
        if not source_name:
            source_name = "unnamed source"
        self._source = cfuncs.create_source(source_name)

    def send(self, midi_data):
        assert isinstance(midi_data, tuple) or isinstance(midi_data, list)
        return cfuncs.send_midi(self._source, midi_data)


class MIDIDestination(object):
    def __init__(self, dest_name=None):
        if not dest_name:
            dest_name = "unnamed destination"
        self._dest = cfuncs.create_destination(dest_name)

    def recv(self):
        return cfuncs.recv_midi(self._dest)


_global_midi_source = None
def _get_global_midi_source():
    global _global_midi_source
    if _global_midi_source is None:
        _global_midi_source = MIDISource("simple core midi source")
    return _global_midi_source


_global_midi_dest = None
def _get_global_midi_dest():
    global _global_midi_dest
    if _global_midi_dest is None:
        _global_midi_dest = MIDIDestination("simple core midi destination")
    return _global_midi_dest


def send_midi(midi_data):
    return _get_global_midi_source().send(midi_data)


def recv_midi():
    return _get_global_midi_dest().recv()
