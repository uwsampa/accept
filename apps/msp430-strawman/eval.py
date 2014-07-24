import re

def parse(lines):
    regfile = {}
    rx = r' +\( ?([A-Z0-9]+): ([0-9a-f]+)\)'
    for line in lines:
        if not line: break
        for match in re.finditer(rx, line):
            reg, val = match.groups()
            regfile[reg] = int(val, 16)
    return regfile

def load():
    return 'file:msp430out.txt'

def score(orig, relaxed):
    def _retval(regs):
        return regs['R15']

    with open(orig, 'r') as origf:
        regs_orig = parse(origf.readlines())
        score_orig = _retval(regs_orig)
    with open(relaxed, 'r') as relaxedf:
        regs_relaxed = parse(relaxedf.readlines())
        score_relaxed = _retval(regs_relaxed)

    return float(abs(score_orig - score_relaxed)) / score_orig

if __name__ == '__main__':
    assert score('testout.txt', 'testout.txt') == 0
