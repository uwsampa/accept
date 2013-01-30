from __future__ import division
from __future__ import print_function
import sys
from collections import namedtuple, Counter

InstructionInfo = namedtuple('InstructionInfo', ['approx', 'elidable'])

def analyze(info_fn="enerc_info.txt",
            trace_fn="enerc_trace.txt"):
    counts = Counter(['dynamic approx', 'dynamic elidable', 'dynamic total',
                      'static approx', 'static elidable', 'static total'])

    # Static information and statistics.
    inst_info = {}
    with open(info_fn) as f:
        for line in f:
            line = line.strip()
            if line:
                num, approx, elidable = line.split()
                approx, elidable = (bool(int(v)) for v in (approx, elidable))
                counts['static approx'] += approx
                counts['static elidable'] += elidable
                counts['static total'] += 1
                inst_info[int(num)] = InstructionInfo(approx, elidable)
    
    # Dynamic statistics.
    with open(trace_fn) as f:
        for line in f:
            line = line.strip()
            if line:
                num = int(line)
                counts['dynamic approx'] += inst_info[num].approx
                counts['dynamic elidable'] += inst_info[num].elidable
                counts['dynamic total'] += 1
    
    return counts

def summarize(counts):
    print('Static instructions: {}'.format(counts['static total']))
    print('{:.2%} approx; {:.2%} elidable'.format(
        counts['static approx'] / counts['static total'],
        counts['static elidable'] / counts['static total'],
    ))
    print()
    print('Dynamic instructions: {}'.format(counts['dynamic total']))
    print('{:.2%} approx; {:.2%} elidable'.format(
        counts['dynamic approx'] / counts['dynamic total'],
        counts['dynamic elidable'] / counts['dynamic total'],
    ))

if __name__ == "__main__":
    args = sys.argv[1:]
    summarize(analyze())
