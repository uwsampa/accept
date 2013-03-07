#!/usr/bin/env python
from __future__ import division
from __future__ import print_function

import os
import sys

def totals(fn):
    approx_total, elidable_total, total_total = 0, 0, 0
    with open(fn) as f:
        for line in f:
            line = line.strip()
            if line:
                approx, elidable, total = map(int, line.split())
                approx_total += approx
                elidable_total += elidable
                total_total += total
    return approx_total, elidable_total, total_total

def summ(fn):
    approx, elidable, total = totals(fn)
    print('{:.2%} approx'.format(approx / total))
    print('{:.2%} elidable'.format(elidable / total))
    print('{} total'.format(total))

if __name__ == '__main__':
    print('Static:')
    try:
        summ('enerc_static.txt')
    except EnvironmentError as err:
        print('Warning: Unable to open file: {}'.format(err))

    print('Dynamic:')
    try:
        summ('enerc_dynamic.txt')
    except EnvironmentError as err:
        print('Warning: Unable to open file: {}'.format(err))
