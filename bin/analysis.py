from __future__ import division
import json
import sys
from collections import namedtuple, Counter

InstructionInfo = namedtuple('InstructionInfo', ['approx', 'elidable'])

def analyze(info_fn="enerc_info.txt",
            trace_fn="enerc_trace.txt"):
    inst_info = {}
    with open(info_fn) as f:
        for line in f:
            line = line.strip()
            if line:
                num, approx, elidable = line.split()
                approx, elidable = (bool(int(v)) for v in (approx, elidable))
                inst_info[int(num)] = InstructionInfo(approx, elidable)
    
    counts = Counter(['approx', 'elidable', 'total'])
    with open(trace_fn) as f:
        for line in f:
            line = line.strip()
            if line:
                num = int(line)

                if inst_info[num].elidable:
                    counts["elidable"] += 1
                if inst_info[num].approx:
                    counts["approx"] += 1
                counts["total"] += 1
    
    counts["proportion approx"] = counts["approx"] / counts["total"]
    counts["proportion elidable"] = counts["elidable"] / counts["total"]
    
    return counts

def _dump(obj):
    json.dump(obj, sys.stdout, indent=4, sort_keys=True)

if __name__ == "__main__":
    args = sys.argv[1:]
    _dump(analyze(*args))
