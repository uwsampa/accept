#!/usr/bin/python

import argparse
import struct

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('infile', help='msp430 accelerometer readings')
    args = parser.parse_args()

    with open(args.infile, 'r') as infile:
        while True:
            bs = infile.read(4)
            if not len(bs):
                break
            readings = struct.unpack('<bbbb', bs)
            print '{{{}}},'.format(', '.join(map(str, readings)))

if __name__ == '__main__':
    main()
