from __future__ import division
from itertools import izip


def load():
    return 'file:output.txt'


def pixels(f):
    for line in f:
        line = line.strip()
        if not line:
            continue
        if 'x' in line:
            # Skip dimensions line.
            continue

        for part in line.split(','):
            yield int(part)


def score(orig, relaxed):
    total_dist = 0
    count = 0

    with open(orig) as orig_f:
        with open(relaxed) as relaxed_f:
            for orig_p, relaxed_p in izip(pixels(orig_f), pixels(relaxed_f)):
                total_dist += abs(orig_p - relaxed_p)
                count += 1

    return total_dist / 255 / count
