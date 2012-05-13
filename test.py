
if __name__=='__main__':
    import simplecoremidi as scm
    scm.setup_midi_output()
    scm.send_midi((0x91, 60, 127))
