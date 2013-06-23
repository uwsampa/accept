from __future__ import division
import subprocess
import re

def load():
    return 'file:out.fluid'

MISMATCH_RE = r'Expected <([-\.\de,]+)>\s+Received <([-\.\de,]+)>'

def euclidean_dist(a, b):
    if len(a) != len(b):
        # Mismatch in vector length. Maximal error.
        return len(a) ** 0.5

    total = 0.0
    for x, y in zip(a, b):
        total += (x - y) ** 2.0
    return total ** 0.5

def score(orig, relaxed):
    output, _ = subprocess.Popen([
        './fluidcmp', relaxed, orig,
        '--verbose', '--ptol', '0',
    ], stdout=subprocess.PIPE).communicate()

    num_particles = int(re.search(r'numParticles: (\d+)', output).group(1))

    total_dist = 0.0
    for expected, received in re.findall(MISMATCH_RE, output):
        evec = []
        rvec = []
        for s, vec in ((expected, evec), (received, rvec)):
            vec[:] = map(float, s.split(','))
        total_dist += euclidean_dist(evec, rvec)

    return total_dist / (3 ** 0.5) / num_particles

# For testing.
if __name__ == '__main__':
    import sys
    print score(sys.argv[1], sys.argv[2])
