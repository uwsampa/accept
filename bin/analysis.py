from __future__ import division
import json
import sys

def analyze(info_fn="enerc_block_info.txt",
            trace_fn="enerc_trace.txt"):
    block_elidable = {}
    with open(info_fn) as f:
        for line in f:
            line = line.strip()
            if line:
                num, elidable = line.split()[:2]
                block_elidable[int(num)] = bool(int(elidable))
    
    counts = {
        "elidable": 0,
        "total": 0,
    }
    with open(trace_fn) as f:
        for line in f:
            line = line.strip()
            if line:
                num = int(line)
                counts["total"] += 1
                if block_elidable[num]:
                    counts["elidable"] += 1
    
    counts["proportion elidable"] = counts["elidable"] / counts["total"]
    
    return counts

def _dump(obj):
    json.dump(obj, sys.stdout, indent=4, sort_keys=True)

if __name__ == "__main__":
    args = sys.argv[1:]
    _dump(analyze(*args))
