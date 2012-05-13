
import time
import simplecoremidi as scm

if __name__=='__main__':
    scm.setup_midi_output()
    for i in range(100):
        scm.send_midi((0x91, 60, 127))
        scm.send_midi((0xB1, 60, 110))
        print "Sent!"
        time.sleep(0.5)
