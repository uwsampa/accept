from __future__ import division
import os
import string
import random
import PIL
import itertools

IMGDIR = 'saved_outputs'

def _random_string(length=20, chars=(string.ascii_letters + string.digits)):
    return ''.join(random.choice(chars) for i in range(length))

def load(fn='out.pgm'):
    if not os.path.isdir(IMGDIR):
        os.mkdir(IMGDIR)
    path = os.path.join(IMGDIR, _random_string() + '.pgm')
    os.rename(fn, path)
    return path

def score(orig, relaxed):
    orig_image = PIL.Image.open(orig)
    relaxed_image = PIL.Image.open(relaxed)
    error = 0
    total = 0
    for ppixel, apixel in itertools.izip(orig_image.getdata,
                                         relaxed_image.getdata()):
        error += abs(ppixel - apixel)
        total += 1
    return error / 255 / total
