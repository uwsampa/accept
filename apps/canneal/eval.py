from __future__ import division

def load(fn='output.txt'):
    with open(fn) as f:
        return int(f.read())

def score(orig, relaxed):
    return 1.0 - relaxed / orig
