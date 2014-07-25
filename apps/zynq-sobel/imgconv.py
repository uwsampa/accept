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
                value = re.search(r'\((\d+),', line).group(1)
                value = int(value)
                row.append(value)
            if len(row) == width:
                print(','.join(map(str, row)))
                del row[:]


if __name__ == '__main__':
    imgconv(sys.argv[1])
