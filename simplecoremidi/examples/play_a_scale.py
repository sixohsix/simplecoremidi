from simplecoremidi import send_midi
from time import sleep

def play_a_scale():
    root_note = 60  # This is middle C
    channel = 1  # This is MIDI channel 1
    note_on_action = 0x90
    major_steps = [2, 2, 1, 2, 2, 2, 1, 0]
    velocity = 127

    note = root_note
    for step in major_steps:
        send_midi((note_on_action | channel,
                   note,
                   velocity))
        sleep(0.1)
        send_midi((note_on_action | channel,
                   note,
                   0))  # A note-off is just a note-on with velocity 0

        note += step
        sleep(0.2)

if __name__=='__main__':
    while True:
        play_a_scale()
