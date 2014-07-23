import re
import pprint
import random # XXX remove

def load():
    return 'file:ar.out'

def parse(lines):
    d = {}
    for line in [x.strip() for x in lines]:
        k, v = line.split()
        d[k] = v
    return d

def score(orig, relaxed):
    def _retval(regs):
        return (regs['R14'] << 16) | regs['R15']
    with open(orig, 'r') as origf:
        results_orig = parse(origf.readlines())
        score_orig = int(results_orig['__NV_movingCount'])
        orig_total = int(results_orig['__NV_totalCount'])
    with open(relaxed, 'r') as relaxedf:
        results_relaxed = parse(relaxedf.readlines())
        score_relaxed = int(results_relaxed['__NV_movingCount'])
        relaxed_total = int(results_relaxed['__NV_totalCount'])
    assert orig_total == relaxed_total

    # XXX scores represent cycle counts.  dump relevant portions of memory and
    # compare contents instead.
    diff = float(abs(score_orig - score_relaxed))
    return diff / orig_total

if __name__ == '__main__':
    assert score('ar.out', 'ar.out') == 0
