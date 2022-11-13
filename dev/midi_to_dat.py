'''
Parse a midi file and write a simplifed .dat file which the TC can play
Only contains Note ON. Note OFF and pitch-bend events

usage:  midi_to_dat.py music.mid
will write output file to dat/music
'''

from mido import MidiFile
from struct import pack
from pathlib import Path
import re
from sys import argv, exit


def main():
    fName = Path(argv[1])
    fOut = fName.parent / 'dat' / clean_name(fName)

    mf = MidiFile(fName)

    # Only support OFF, ON and BEND events
    ev_cnts = {0x80: 0, 0x90: 0, 0xE0: 0}
    n_filtered = 0
    active_notes = set()
    dt = 0.0

    with open(fOut, 'wb') as f:
        for i, ev in enumerate(mf):
            dt += ev.time
            evb = ev.bin()

            # note_on with 0 velocity is the same as note_off
            if evb[0] == 0x90 and evb[2] == 0:
                evb[0] = 0x80

            # skip all non relevant events
            if evb[0] not in ev_cnts:
                # print(f'{i:4d}', ev)
                continue

            if evb[0] == 0x90:  # note on
                # don't turn ON notes which are already active
                if evb[1] in active_notes:
                    n_filtered += 1
                    continue
                active_notes.add(evb[1])
            elif evb[0] == 0x80:  # note off
                if evb[1] in active_notes:
                    active_notes.remove(evb[1])
                else:
                    # don't turn OFF notes which are not active
                    n_filtered += 1
                    continue

            # Write events to simple .dat file
            # Make sure there are no delays before the first note-ON event
            dt_i = 0 if ev_cnts[0x90] == 0 else int(dt * 1000) & 0xFFFF
            dt = 0.0

            dat = pack('<HBBB', dt_i, *evb)
            f.write(dat)

            ev_cnts[evb[0]] += 1

    for k, v in ev_cnts.items():
        print(f'{k:02x}: {v:6d},', end=' ')
    print(f'filtered: {n_filtered:4d},', end=' ')
    print(fOut)


def clean_name(fn):
    n = fn.with_suffix('').name
    n = re.sub(r'\W|^(?=\d)', '', n)
    n = n[:30]
    return n


if __name__ == '__main__':
    if len(argv) != 2:
        print(__doc__)
        exit(-1)

    main()
