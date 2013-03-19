from __future__ import division
import PIL
import PIL.Image
import itertools

IMGDIR = 'saved_outputs'

def load():
    return 'file:out.pgm'

def score(orig, relaxed):
    orig_image = PIL.Image.open(orig)
    relaxed_image = PIL.Image.open(relaxed)
    error = 0
    total = 0

    try:
        orig_data = orig_image.getdata()
        relaxed_data = relaxed_image.getdata()
    except ValueError:
        return 1.0

    for ppixel, apixel in itertools.izip(orig_data, relaxed_data):
        error += abs(ppixel - apixel)
        total += 1
    return error / 255 / total
