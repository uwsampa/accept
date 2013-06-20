from __future__ import division
from math import sqrt
import PIL
import PIL.Image
import itertools

IMGDIR = 'saved_outputs'

def load():
    return 'file:baboon.rgb.jpg'

def score(orig, relaxed):
    print 'Orig: %s' % orig
    orig_image = PIL.Image.open(orig)
    print 'Relaxed: %s' % relaxed
    relaxed_image = PIL.Image.open(relaxed)
    error = 0
    total = 0

    try:
        orig_data = orig_image.getdata()
        relaxed_data = relaxed_image.getdata()
    except ValueError:
        return 1.0

    for ppixel, apixel in itertools.izip(orig_data, relaxed_data):
        # root-mean-square error per pixel
        pxerr = sqrt(((ppixel[0] - apixel[0])**2 +
                      (ppixel[1] - apixel[1])**2 +
                      (ppixel[2] - apixel[2])**2) / 3) / 255
        error += 1.0 - pxerr
        total += 1
    return error / total
