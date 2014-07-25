from __future__ import print_function
import subprocess
import sys
import re


def imgconv(image_fn):
    text_fn = image_fn + '.txt'
    subprocess.check_call(['convert', image_fn, text_fn])
    with open(text_fn) as f:
        meta = f.readline()
        _, meta = meta.rsplit(None, 1)
        width, height, depth, mode = meta.split(',')
        width = int(width)
        height = int(height)
        print('{},{}'.format(width, height))

        row = []
        for line in f:
            line = line.strip()
            if line:
                values = re.findall(r'\d+', line)
                r, g, b = map(int, values[2:5])
                row += [r, g, b]
            if len(row) == width * 3:
                print(','.join(map(str, row)))
                del row[:]

        print("\"{'bitdepth': 8, 'interlace': 0, 'planes': 3, "
              "'greyscale': False, 'alpha': False, 'size': (256, 256)}\"",
              end='')


if __name__ == '__main__':
    imgconv(sys.argv[1])
