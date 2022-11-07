'''
Parse a midi file and write a simplifed .dat file which the TC can play
Only contains Note ON. Note OFF and pitch-bend events

usage:  midi_to_dat.py music.mid
will write music.dat in the same directory
'''

from MIDI import MIDIFile, Events
from struct import pack
from pathlib import Path
import re
from sys import argv, exit


def main():
    # for fName in glob('midi_files/2_notes/*'):
    fName = Path(argv[1])
    # print(fName.name)
    mf = MIDIFile(fName)
    mf.parse()
    # print(mf)

    ev_cnts = {0x80: 0, 0x90: 0, 0xA0: 0, 0xE0: 0}
    all_ev = []
    for idx, track in enumerate(mf):
        track.parse()
        # print(f'-------- Track {idx} --------')
        for ev in track:
            # print(ev)
            if isinstance(ev, Events.MIDIEvent):
                if ev.command in ev_cnts:
                    all_ev.append(ev)

    fOut = clean_name(fName).with_suffix('.dat')
    with open(fOut, 'wb') as f:
        t0 = None
        ev_ = None
        for i, ev in enumerate(sorted(all_ev, key=lambda x: x.time)):
            if i == 0:
                t0 = ev.time
            else:
                # Filter out duplicate events
                if str(ev) == str(ev_):
                    continue
            ev.time -= t0

            # Write note events to simple .dat file
            dat = pack('HBBB', ev.time & 0xFFFF, ev.command, *ev.data)
            f.write(dat)

            ev_cnts[ev.command] += 1
            ev_ = ev

    for k, v in ev_cnts.items():
        print(f'{k:02x}: {v:6d},', end=' ')
    print(fOut)


def clean_name(fn):
    n = fn.with_suffix('').name
    n = re.sub('\W|^(?=\d)','', n)
    n = n[:27]
    return fn.with_name(n).with_suffix(fn.suffix)


if __name__ == '__main__':
    if len(argv) != 2:
        print(__doc__)
        exit(-1)

    main()
