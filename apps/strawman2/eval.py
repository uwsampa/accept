from __future__ import division
from random import random

def load():
    return 'file:output.txt'

def score(orig, relaxed):
    return 0.75 + random() / 4
