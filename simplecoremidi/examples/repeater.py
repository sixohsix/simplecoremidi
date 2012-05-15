
from time import time, sleep
from simplecoremidi import MIDISource, MIDIDestination

LOOP_WAIT = 1.0 / 200  # 200 Hz
DELAY = 1  # 1 sec

def repeater():
    dest = MIDIDestination("repeater input")
    source = MIDISource("repeater output")

    frames = []

    last_frame_time = time()
    while True:
        midi_in = dest.recv()
        if midi_in:
            #source.send(midi_in)
            frames.append((last_frame_time + DELAY, midi_in))
        while frames:
            midi_out = frames[0]
            if midi_out[0] < last_frame_time:
                source.send(midi_out[1])
                frames.pop(0)
            else:
                break

        now = time()
        wait_time = -1
        while wait_time <= 0:
            last_frame_time = last_frame_time + LOOP_WAIT
            wait_time = last_frame_time - now
        sleep(wait_time)

if __name__=='__main__':
    repeater()
