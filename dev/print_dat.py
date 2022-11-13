from sys import argv
from struct import unpack, calcsize

fmt = '<HBBB'
s = calcsize(fmt)
i = 0
active_notes = set()

with open(argv[1], 'rb') as f:
    while True:
        dat = f.read(s)
        if len(dat) < s:
            break

        t, *evb = unpack(fmt, dat)

        if evb[0] == 0x90:  # note on
            active_notes.add(evb[1])
        elif evb[0] == 0x80:  # note off
            active_notes.remove(evb[1])

        x = {0x90: '+', 0x80: '-', 0xE0: 'b'}[evb[0]]
        print(f'{i:4d} {t:4x} {x} {evb[1]:3d}', active_notes)
        i += 1

        # if i > 64:
        #     break
